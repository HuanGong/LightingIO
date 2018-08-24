

#include "tcp_channel.h"

#include "base/base_constants.h"
#include "glog/logging.h"
#include "base/closure/closure_task.h"
#include "protocol/proto_service.h"

namespace net {


//static
RefTcpChannel TcpChannel::Create(int socket_fd,
                                 const InetAddress& local,
                                 const InetAddress& peer,
                                 base::MessageLoop* loop,
                                 ChannelServeType type) {

  return std::make_shared<TcpChannel>(socket_fd, local, peer, loop, type);
}

RefTcpChannel TcpChannel::CreateServerChannel(int socket_fd,
                                              const InetAddress& local,
                                              const InetAddress& peer,
                                              base::MessageLoop* loop) {

  return std::make_shared<TcpChannel>(socket_fd, local, peer, loop, kServerType);
}

RefTcpChannel TcpChannel::CreateClientChannel(int socket_fd,
                                              const InetAddress& local,
                                              const InetAddress& remote,
                                              base::MessageLoop* loop) {

  return std::make_shared<TcpChannel>(socket_fd, local, remote, loop, kClientType);
}

TcpChannel::TcpChannel(int socket_fd,
                       const InetAddress& loc,
                       const InetAddress& peer,
                       base::MessageLoop* loop,
                       ChannelServeType type)
  : work_loop_(loop),
    owner_loop_(NULL),
    channel_status_(CONNECTING),
    local_addr_(loc),
    peer_addr_(peer),
    serve_type_(type) {

  CHECK(work_loop_);

  fd_event_ = base::FdEvent::Create(socket_fd, 0);
  fd_event_->SetReadCallback(std::bind(&TcpChannel::HandleRead, this));
  fd_event_->SetWriteCallback(std::bind(&TcpChannel::HandleWrite, this));
  fd_event_->SetCloseCallback(std::bind(&TcpChannel::HandleClose, this));
  fd_event_->SetErrorCallback(std::bind(&TcpChannel::HandleError, this));
  socketutils::KeepAlive(fd_event_->fd(), true);
}

void TcpChannel::Start() {
  CHECK(fd_event_);
  CHECK(proto_service_);
  if (work_loop_->IsInLoopThread()) {
    return OnConnectionReady();
  }

  auto task = std::bind(&TcpChannel::OnConnectionReady, shared_from_this());
  work_loop_->PostTask(base::NewClosure(std::move(task)));
}

TcpChannel::~TcpChannel() {
  VLOG(GLOG_VTRACE) << channal_name_ << " Gone, Fd:" << fd_event_->fd();
  fd_event_.reset();
  CHECK(channel_status_ == DISCONNECTED);
}

void TcpChannel::OnConnectionReady() {
  CHECK(InIOLoop());

  if (channel_status_ != CONNECTING) {
    LOG(ERROR) << " Channel Status Changed After Enable this Channel";
    if (closed_callback_) {
      RefTcpChannel guard(shared_from_this());
      closed_callback_(guard);
    }
    fd_event_->ResetCallback();
    return;
  }

  base::EventPump* event_pump = work_loop_->Pump();
  event_pump->InstallFdEvent(fd_event_.get());
  fd_event_->EnableReading();
  SetChannelStatus(CONNECTED);
}

void TcpChannel::HandleRead() {
  int error = 0;

  int32_t bytes_read = in_buffer_.ReadFromSocketFd(fd_event_->fd(), &error);
  VLOG(GLOG_VTRACE) << " Read " << bytes_read << " bytes from:" << channal_name_;

  if (bytes_read > 0) {

    proto_service_->OnDataRecieved(shared_from_this(), &in_buffer_);
    if ( recv_data_callback_ ) {
      recv_data_callback_(shared_from_this(), &in_buffer_);
    }

  } else if (0 == bytes_read) { //read eof, remote close

    HandleClose();

  } else {

    errno = error;
    HandleError();

    HandleClose();
  }
}

void TcpChannel::HandleWrite() {

  int socket_fd = fd_event_->fd();

  if (!fd_event_->IsWriteEnable()) {
    VLOG(GLOG_VTRACE) << "Connection Writen is disabled, fd:" << socket_fd;
    return;
  }

  int32_t data_size = out_buffer_.CanReadSize();
  if (0 == data_size) {
    fd_event_->DisableWriting();

    if (schedule_shutdown_) {
      socketutils::ShutdownWrite(socket_fd);
    }
    return;
  }
  int writen_bytes = socketutils::Write(socket_fd, out_buffer_.GetRead(), data_size);

  if (writen_bytes > 0) {
    out_buffer_.Consume(writen_bytes);

    if (out_buffer_.CanReadSize() == 0) { //all data writen
      if (finish_write_callback_) {
        finish_write_callback_(shared_from_this());
      }
      proto_service_->OnDataFinishSend(shared_from_this());
    }
  } else {
    LOG(ERROR) << "Call Socket Write Error";
  }

  if (out_buffer_.CanReadSize() == 0) { //all data writen
    fd_event_->DisableWriting();

    if (schedule_shutdown_) {
      socketutils::ShutdownWrite(fd_event_->fd());
    }
  }
}

bool TcpChannel::SendProtoMessage(RefProtocolMessage message) {
  CHECK(message);
  CHECK(work_loop_->IsInLoopThread());

  if (channel_status_ != ChannelStatus::CONNECTED) {
    VLOG(GLOG_VERROR) << "Channel Is Broken, ChannelInfo:" << ChannelName() << " Status:" << StatusAsString();
    return false;
  }

  proto_service_->BeforeSendMessage(message.get());

  bool success = false;
  if (out_buffer_.CanReadSize()) {

    success = proto_service_->EncodeToBuffer(message.get(), &out_buffer_);
    LOG_IF(ERROR, !success) << "Send Failed For Message Encode Failed";

    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }

  } else { //out_buffer_ empty, try write directly

    IOBuffer buffer;
    success = proto_service_->EncodeToBuffer(message.get(), &buffer);
    LOG_IF(ERROR, !success) << "Send Failed For Message Encode Failed";
    if (success && buffer.CanReadSize()) {
      Send(buffer.GetRead(), buffer.CanReadSize());
    }
  }
  return success;
}

void TcpChannel::HandleError() {
  int socket_fd = fd_event_->fd();
  int err = socketutils::GetSocketError(socket_fd);
  thread_local static char t_err_buff[128];
  LOG(ERROR) << " Socket Error, fd:[" << socket_fd << "], error info: [" << strerror_r(err, t_err_buff, sizeof t_err_buff) << "]";
}

void TcpChannel::HandleClose() {
  CHECK(work_loop_->IsInLoopThread());

  RefTcpChannel guard(shared_from_this());
  //if (channel_status_ == DISCONNECTED) {
    //return;
  //}

  fd_event_->DisableAll();
  fd_event_->ResetCallback();
  work_loop_->Pump()->RemoveFdEvent(fd_event_.get());

  VLOG(GLOG_VTRACE) << "HandleClose, Channel:" << ChannelName() << " Status: DISCONNECTED";

  SetChannelStatus(DISCONNECTED);

  if (closed_callback_) {
    closed_callback_(guard);
  }
}

void TcpChannel::SetChannelStatus(ChannelStatus st) {
  channel_status_ = st;

  RefTcpChannel guard(shared_from_this());
  if (status_change_callback_) {
    status_change_callback_(guard);
  }
  proto_service_->OnStatusChanged(guard);
}

void TcpChannel::ShutdownChannel() {
  CHECK(InIOLoop());

  VLOG(GLOG_VTRACE) << "TcpChannel::ShutdownChannel " << channal_name_;
  if (channel_status_ == DISCONNECTED) {
    return;
  }

  if (!fd_event_->IsWriteEnable()) {
    SetChannelStatus(DISCONNECTING);
    socketutils::ShutdownWrite(fd_event_->fd());
  } else {
    schedule_shutdown_ = true;
  }
}

void TcpChannel::ForceShutdown() {
  CHECK(InIOLoop());

  HandleClose();
}

int32_t TcpChannel::Send(const uint8_t* data, const int32_t len) {
  CHECK(work_loop_->IsInLoopThread());

  if (channel_status_ != CONNECTED) {
    VLOG(GLOG_VERROR) << "Can't Write Data To a Closed[ing] socket; Channal:" << ChannelName();
    return -1;
  }

  int32_t n_write = 0;
  int32_t n_remain = len;

  //nothing write in out buffer
  if (0 == out_buffer_.CanReadSize()) {

    n_write = socketutils::Write(fd_event_->fd(), data, len);

    if (n_write >= 0) {

      n_remain = len - n_write;

      if (n_remain == 0) { //finish all

        RefTcpChannel guard(shared_from_this()); 
        proto_service_->OnDataFinishSend(guard);
        if (finish_write_callback_) {
          finish_write_callback_(guard);
        }

      }
    } else { //n_write < 0
      n_write = 0;
      int32_t err = errno;

      if (EAGAIN != err) {
        LOG(ERROR) << "Send Data Fail, fd:[" << fd_event_->fd() << "] errno: [" << err << "]";
      }
    }
  }

  if (n_remain > 0) {
    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }
    out_buffer_.WriteRawData(data + n_write, n_remain);
  }
  return n_write;
}

void TcpChannel::SetChannelName(const std::string name) {
  channal_name_ = name;
}

void TcpChannel::SetOwnerLoop(base::MessageLoop* owner) {
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

ProtoService* TcpChannel::GetProtoService() const {
  return proto_service_.get();
}

void TcpChannel::SetProtoService(ProtoServicePtr&& proto_service) {
  proto_service_ = std::move(proto_service);
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

base::MessageLoop* TcpChannel::IOLoop() const {
  return work_loop_;
}
bool TcpChannel::InIOLoop() const {
  return work_loop_->IsInLoopThread();
}

bool TcpChannel::IsClientChannel() const {
  return serve_type_ == ChannelServeType::kClientType;
}

bool TcpChannel::IsServerChannel() const {
  return serve_type_ == ChannelServeType::kServerType;
}

bool TcpChannel::IsConnected() const {
  return channel_status_ == CONNECTED;
}

}
