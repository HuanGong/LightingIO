

#include "tcp_channel.h"

#include "base/base_constants.h"
#include "glog/logging.h"
#include "base/closure/closure_task.h"

namespace net {

//static
RefTcpChannel TcpChannel::Create(int socket_fd,
                                 const InetAddress& local,
                                 const InetAddress& peer,
                                 base::MessageLoop2* loop) {

  RefTcpChannel conn(new TcpChannel(socket_fd,
                                    local,
                                    peer,
                                    loop));
  conn->Initialize();
  return std::move(conn);
}

TcpChannel::TcpChannel(int socket_fd,
                       const InetAddress& loc,
                       const InetAddress& peer,
                       base::MessageLoop2* loop)
  : work_loop_(loop),
    owner_loop_(NULL),
    channel_status_(CONNECTING),
    socket_fd_(socket_fd),
    local_addr_(loc),
    peer_addr_(peer) {

  fd_event_ = base::FdEvent::create(socket_fd_, 0);
}

void TcpChannel::Initialize() {
  //conside move follow to InitConnection ensure enable_shared_from_this work
  base::EventPump* event_pump = work_loop_->Pump();
  fd_event_->SetDelegate(event_pump->AsFdEventDelegate());
  fd_event_->SetReadCallback(std::bind(&TcpChannel::HandleRead, this));
  fd_event_->SetWriteCallback(std::bind(&TcpChannel::HandleWrite, this));
  fd_event_->SetCloseCallback(std::bind(&TcpChannel::HandleClose, this));
  fd_event_->SetErrorCallback(std::bind(&TcpChannel::HandleError, this));

  socketutils::KeepAlive(socket_fd_, true);

  auto task = std::bind(&TcpChannel::OnConnectionReady, shared_from_this());
  work_loop_->PostTask(base::NewClosure(std::move(task)));
}

TcpChannel::~TcpChannel() {
  VLOG(GLOG_VTRACE) << "TcpChannel Gone, Fd:" << socket_fd_ << " status:" << StatusToString();
  //CHECK(channel_status_ == DISCONNECTED);
  LOG(INFO) << " TcpChannel::~TcpChannel Gone";
  socketutils::CloseSocket(socket_fd_);
}

void TcpChannel::OnConnectionReady() {

  if (channel_status_ == CONNECTING) {

    base::EventPump* event_pump = work_loop_->Pump();
    event_pump->InstallFdEvent(fd_event_.get());
    fd_event_->EnableReading();
    //fd_event_->EnableWriting();

    channel_status_ = CONNECTED;
    OnStatusChanged();
  } else {
    LOG(INFO) << "This Connection Status Changed After Initialize";
  }
}

void TcpChannel::OnStatusChanged() {
  if (status_change_callback_) {
    RefTcpChannel guard(shared_from_this());
    status_change_callback_(guard);
  }
}

void TcpChannel::HandleRead() {
  int error = 0;

  int32_t bytes_read = in_buffer_.ReadFromSocketFd(socket_fd_, &error);

  if (bytes_read > 0) {
    if ( recv_data_callback_ ) {
      recv_data_callback_(shared_from_this(), &in_buffer_);
    }
  } else if (0 == bytes_read) { //read eof
    HandleClose();
  } else {
    errno = error;
    HandleError();
  }
}

void TcpChannel::HandleWrite() {

  if (!fd_event_->IsWriteEnable()) {
    VLOG(GLOG_VTRACE) << "Connection Writen is disabled, fd:" << socket_fd_;
    return;
  }

  int32_t data_size = out_buffer_.CanReadSize();
  if (0 == data_size) {
    LOG(INFO) << " Noting Need Write In Buffer, fd: " << socket_fd_;
    fd_event_->DisableWriting();
    return;
  }
  int writen_bytes = socketutils::Write(socket_fd_,
                                        out_buffer_.GetRead(),
                                        data_size);

  if (writen_bytes > 0) {
    out_buffer_.Consume(writen_bytes);
    if (out_buffer_.CanReadSize() == 0) { //all data writen

      //callback this may write data again
      if (finish_write_callback_) {
        finish_write_callback_(shared_from_this());
      }
    }
  } else {
    LOG(ERROR) << "Call Socket Write Error";
  }

  if (out_buffer_.CanReadSize() == 0) { //all data writen
    fd_event_->DisableWriting();
  }
}

void TcpChannel::HandleError() {
  int err = socketutils::GetSocketError(socket_fd_);
  thread_local static char t_err_buff[128];
  LOG(ERROR) << " Socket Error, fd:[" << socket_fd_ << "], error info: [" << strerror_r(err, t_err_buff, sizeof t_err_buff) << " ]";
}

void TcpChannel::HandleClose() {

  CHECK(work_loop_->IsInLoopThread());
  if (channel_status_ == DISCONNECTED) {
    return;
  }

  channel_status_ = DISCONNECTED;

  fd_event_->DisableAll();
  work_loop_->Pump()->RemoveFdEvent(fd_event_.get());

  RefTcpChannel guard(shared_from_this());

  OnStatusChanged();

  // normal case, it will remove from connection's ownner
  // after this, it's will destructor if no other obj hold it
  if (closed_callback_) {
    if (owner_loop_) {
      owner_loop_->PostTask(base::NewClosure(std::bind(closed_callback_, guard)));
    } else {
      closed_callback_(guard);
    }
  }
}

std::string TcpChannel::StatusToString() const {
  switch (channel_status_) {
    case CONNECTED:
      return "CONNECTED";
    case DISCONNECTING:
      return "DISCONNECTING";
    case DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UnKnown";
  }
  return "UnKnown";
}

void TcpChannel::ShutdownChannel() {
  if (!work_loop_->IsInLoopThread()) {
    auto f = std::bind(&TcpChannel::ShutdownChannel, shared_from_this());
    work_loop_->PostTask(base::NewClosure(std::move(f)));
    return;
  }
  if (!fd_event_->IsWriteEnable()) {
    socketutils::ShutdownWrite(socket_fd_);
  }
}

int32_t TcpChannel::Send(const uint8_t* data, const int32_t len) {
  CHECK(work_loop_->IsInLoopThread());

  if (channel_status_ != CONNECTED) {
    LOG(INFO) <<  "Can't Write Data To a Closed[ing] socket" << channel_status_;
    return -1;
  }

  int32_t n_write = 0;
  int32_t n_remain = len;

  //nothing write in out buffer
  if (0 == out_buffer_.CanReadSize()) {
    n_write = socketutils::Write(socket_fd_, data, len);

    if (n_write >= 0) {

      n_remain = len - n_write;

      if (n_remain == 0 && finish_write_callback_) { //finish all
        finish_write_callback_(shared_from_this());
      }
    } else { //n_write < 0
      n_write = 0;
      int32_t err = errno;

      if (EAGAIN != err) {
        LOG(ERROR) << "Send Data Fail, fd:[" << socket_fd_ << "] errno: [" << err << "]";
      }
    }
  }

  if ( n_remain > 0) {
    out_buffer_.WriteRawData(data + n_write, n_remain);

    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }
  }
  return n_write;
}

void TcpChannel::SetChannelName(const std::string name) {
  channal_name_ = name;
}
void TcpChannel::SetOwnerLoop(base::MessageLoop2* owner) {
  owner_loop_ = owner;
}
void TcpChannel::SetDataHandleCallback(const RcvDataCallback& callback) {
  recv_data_callback_ = callback;
}
void TcpChannel::SetFinishSendCallback(const FinishSendCallback& callback) {
  finish_write_callback_ = callback;
}
void TcpChannel::SetCloseCallback(const ChannelClosedCallback& callback) {
  closed_callback_ = callback;
}
void TcpChannel::SetStatusChangedCallback(const ChannelStatusCallback& callback) {
  status_change_callback_ = callback;
}
const std::string TcpChannel::StatusAsString() {
  switch(channel_status_) {
    case CONNECTING:
      return "CONNECTING";
    case CONNECTED:
      return "ESTABLISHED";
    case DISCONNECTING:
      return "DISCONNECTING";
    case DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
  return "UNKNOWN";
}

}
