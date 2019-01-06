

#include "tcp_channel.h"

#include "base/base_constants.h"
#include "glog/logging.h"
#include "base/closure/closure_task.h"
#include <base/utils/sys_error.h>

namespace net {


//static
RefTcpChannel TcpChannel::Create(int socket_fd,
                                 const SocketAddress& local,
                                 const SocketAddress& peer,
                                 base::MessageLoop* loop) {

  return std::make_shared<TcpChannel>(socket_fd, local, peer, loop);
}

TcpChannel::TcpChannel(int socket_fd,
                       const SocketAddress& loc,
                       const SocketAddress& peer,
                       base::MessageLoop* loop)
  : io_loop_(loop),
    local_addr_(loc),
    peer_addr_(peer) {

  CHECK(io_loop_);

  name_ = local_addr_.IpPort();
  fd_event_ = base::FdEvent::Create(socket_fd, 0);
  fd_event_->SetReadCallback(std::bind(&TcpChannel::HandleRead, this));
  fd_event_->SetWriteCallback(std::bind(&TcpChannel::HandleWrite, this));
  fd_event_->SetCloseCallback(std::bind(&TcpChannel::HandleClose, this));
  fd_event_->SetErrorCallback(std::bind(&TcpChannel::HandleError, this));
  socketutils::KeepAlive(fd_event_->fd(), true);
}

void TcpChannel::Start() {
  CHECK(fd_event_);
  status_ = Status::CONNECTING;

  auto t = std::bind(&TcpChannel::OnChannelReady, shared_from_this());

  io_loop_->PostTask(NewClosure(std::move(t)));
}

TcpChannel::~TcpChannel() {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();
  CHECK(status_ == Status::CLOSED);
}

std::string TcpChannel::ChannelInfo() const {
  std::ostringstream oss;
  oss << " [name:" << name_
      << ", fd:" << fd_event_->fd()
      << ", loc:" << local_addr_.IpPort()
      << ", peer:" << peer_addr_.IpPort()
      << ", status:" << StatusAsString() << "]";
  return oss.str();
}

void TcpChannel::OnChannelReady() {
  CHECK(InIOLoop());
  CHECK(channel_consumer_);

  if (status_ != Status::CONNECTING) {
    LOG(ERROR) << __FUNCTION__ << ChannelInfo();
    RefTcpChannel guard(shared_from_this());

    fd_event_->ResetCallback();
    SetChannelStatus(Status::CLOSED);
    channel_consumer_->OnChannelClosed(guard);
    return;
  }

  fd_event_->EnableReading();
  base::EventPump* event_pump = io_loop_->Pump();
  event_pump->InstallFdEvent(fd_event_.get());
  SetChannelStatus(Status::CONNECTED);
}

void TcpChannel::SetChannelConsumer(ChannelConsumer *consumer) {
  channel_consumer_ = consumer;
}

void TcpChannel::HandleRead() {
  static const int32_t block_size = 4 * 1024;
  do {
    in_buffer_.EnsureWritableSize(block_size);

    ssize_t bytes_read = ::read(fd_event_->fd(), in_buffer_.GetWrite(), in_buffer_.CanWriteSize());
    VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo() << " read [" << bytes_read << "] bytes";

    if (bytes_read > 0) {

      in_buffer_.Produce(bytes_read);
      channel_consumer_->OnDataReceived(shared_from_this(), &in_buffer_);

    } else if (0 == bytes_read) {

      HandleClose();
      break;

    } else if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      LOG(ERROR) << __FUNCTION__ << ChannelInfo() << " read error:" << base::StrError();
      HandleClose();
      break;
    }
  } while(1);
}

void TcpChannel::HandleWrite() {
  int fatal_err = 0;
  int socket_fd = fd_event_->fd();

  while(out_buffer_.CanReadSize()) {

    ssize_t writen_bytes = socketutils::Write(socket_fd, out_buffer_.GetRead(), out_buffer_.CanReadSize());
    if (writen_bytes < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        fatal_err = errno;
      }
      break;
    }
    out_buffer_.Consume(writen_bytes);
  };

  if (fatal_err != 0) {
    LOG(ERROR) << __FUNCTION__ << ChannelInfo() << " write err:" << base::StrError(fatal_err);
    HandleClose();
	  return;
  }

  if (out_buffer_.CanReadSize() == 0) {

    fd_event_->DisableWriting();
    channel_consumer_->OnDataFinishSend(shared_from_this());

    if (schedule_shutdown_) {
      HandleClose();
    }
  }
}

void TcpChannel::HandleError() {

  int err = socketutils::GetSocketError(fd_event_->fd());
  LOG(ERROR) << __FUNCTION__ << ChannelInfo() << " error: [" << base::StrError(err) << "]";

  HandleClose();
}

void TcpChannel::HandleClose() {
  DCHECK(io_loop_->IsInLoopThread());

  RefTcpChannel guard(shared_from_this());
  if (Status() == Status::CLOSED) {
    return;
  }
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();

	io_loop_->Pump()->RemoveFdEvent(fd_event_.get());
	fd_event_->ResetCallback();
  SetChannelStatus(Status::CLOSED);

  channel_consumer_->OnChannelClosed(guard);
}

void TcpChannel::SetChannelStatus(Status st) {
  status_ = st;
  RefTcpChannel guard(shared_from_this());
  channel_consumer_->OnStatusChanged(guard);
}

void TcpChannel::ShutdownChannel() {
  CHECK(InIOLoop());

  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();

  if (status_ == Status::CLOSED) {
    return;
  }

  schedule_shutdown_ = true;
  SetChannelStatus(Status::CLOSING);

  if (!fd_event_->IsWriteEnable()) {
    HandleClose();
    schedule_shutdown_ = false;
  }
}

int32_t TcpChannel::Send(const uint8_t* data, const int32_t len) {
  DCHECK(io_loop_->IsInLoopThread());

  if (!IsConnected()) {
    return -1;
  }

  if (out_buffer_.CanReadSize() > 0) {
    out_buffer_.WriteRawData(data, len);

    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }
    return 0; //all write to buffer, zero write to fd
  }

  int32_t fatal_err = 0;
  size_t n_write = 0;
  size_t n_remain = len;

  do {
    ssize_t part_write = socketutils::Write(fd_event_->fd(), data + n_write, n_remain);

    if (part_write < 0) {

      if (errno == EAGAIN || errno == EWOULDBLOCK) {

        out_buffer_.WriteRawData(data + n_write, n_remain);

      } else {
        // to avoid A -> B -> callback  delete A problem, we use a writecallback handle this
      	fatal_err = errno;
        LOG(ERROR) << __FUNCTION__ << ChannelInfo() << " send err:" << base::StrError() << " schedule close channel";
        schedule_shutdown_ = true;
      }
      break;
    }
    n_write += part_write;
    n_remain = n_remain - part_write;

    DCHECK((n_write + n_remain) == size_t(len));
  } while(n_remain != 0);

  if (!fd_event_->IsWriteEnable() && (out_buffer_.CanReadSize() || schedule_shutdown_)) {
    fd_event_->EnableWriting();
  }
  return fatal_err != 0 ? -1 : n_write;
}

const std::string TcpChannel::StatusAsString() const {
  switch(status_) {
    case Status::CONNECTING:
      return "CONNECTING";
    case Status::CONNECTED:
      return "ESTABLISHED";
    case Status::CLOSING:
      return "CLOSING";
    case Status::CLOSED:
      return "CLOSED";
    default:
    	break;
  }
  return "UNKNOWN";
}

base::MessageLoop* TcpChannel::IOLoop() const {
  return io_loop_;
}
bool TcpChannel::InIOLoop() const {
  return io_loop_->IsInLoopThread();
}

bool TcpChannel::IsConnected() const {
  return status_ == Status::CONNECTED;
}

}
