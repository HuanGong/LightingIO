

#include "tcp_channel.h"

#include "base/base_constants.h"
#include "glog/logging.h"
#include "base/closure/closure_task.h"
#include "protocol/proto_service.h"

namespace net {

RefTcpChannel TcpChannel::CreateClientChannel(int socket_fd,
                                              const InetAddress& local,
                                              const InetAddress& remote,
                                              base::MessageLoop* loop) {

  RefTcpChannel conn(new TcpChannel(socket_fd, local, remote,
                                    loop, ChannelServeType::kClientType));
  conn->Initialize();
  return std::move(conn);
}

//static
RefTcpChannel TcpChannel::Create(int socket_fd,
                                 const InetAddress& local,
                                 const InetAddress& peer,
                                 base::MessageLoop* loop,
                                 ChannelServeType type) {

  RefTcpChannel conn(new TcpChannel(socket_fd,
                                    local,
                                    peer,
                                    loop,
                                    type));
  conn->Initialize();
  return std::move(conn);
}

RefTcpChannel TcpChannel::CreateServerChannel(int socket_fd,
                                              const InetAddress& local,
                                              const InetAddress& peer,
                                              base::MessageLoop* loop) {

  RefTcpChannel conn(new TcpChannel(socket_fd, local, peer, loop,
                                    ChannelServeType::kServerType));
  conn->Initialize();
  return std::move(conn);
}

TcpChannel::TcpChannel(int socket_fd,
                       const InetAddress& loc,
                       const InetAddress& peer,
                       base::MessageLoop* loop,
                       ChannelServeType type)
  : work_loop_(loop),
    owner_loop_(NULL),
    channel_status_(CONNECTING),
    socket_fd_(socket_fd),
    local_addr_(loc),
    peer_addr_(peer),
    serve_type_(type) {

  CHECK(work_loop_);
}

void TcpChannel::Initialize() {

  fd_event_ = base::FdEvent::create(socket_fd_, 0);
  fd_event_->SetReadCallback(std::bind(&TcpChannel::HandleRead, this));
  fd_event_->SetWriteCallback(std::bind(&TcpChannel::HandleWrite, this));
  fd_event_->SetCloseCallback(std::bind(&TcpChannel::HandleClose, this));
  fd_event_->SetErrorCallback(std::bind(&TcpChannel::HandleError, this));
  socketutils::KeepAlive(socket_fd_, true);
}

void TcpChannel::Start() {
  CHECK(fd_event_);

  auto task = std::bind(&TcpChannel::OnConnectionReady, shared_from_this());
  work_loop_->PostTask(base::NewClosure(std::move(task)));
}

TcpChannel::~TcpChannel() {
  VLOG(GLOG_VTRACE) << channal_name_ << " Gone, Fd:" << socket_fd_;
  LOG(INFO) << channal_name_ << " Gone, Fd:" << socket_fd_;

  CHECK(channel_status_ == DISCONNECTED);
  if (socket_fd_ != -1) {
    socketutils::CloseSocket(socket_fd_);
  }
}

void TcpChannel::OnConnectionReady() {
  CHECK(InIOLoop());
  if (channel_status_ == CONNECTING) {

    base::EventPump* event_pump = work_loop_->Pump();
    event_pump->InstallFdEvent(fd_event_.get());
    fd_event_->EnableReading();

    SetChannelStatus(CONNECTED);
  } else {

    fd_event_.reset();
    if (socket_fd_ != -1) {
      socketutils::CloseSocket(socket_fd_);
      socket_fd_ = -1;
    }
    LOG(ERROR) << " Channel Status Changed After Enable this Channel";
  }
}

void TcpChannel::HandleRead() {
  int error = 0;

  int32_t bytes_read = in_buffer_.ReadFromSocketFd(socket_fd_, &error);
  VLOG(GLOG_VTRACE) << " Read " << bytes_read << " bytes from:" << channal_name_;

  if (bytes_read > 0) {
    proto_service_->OnDataRecieved(shared_from_this(), &in_buffer_);
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
    fd_event_->DisableWriting();

    if (schedule_shutdown_) {
      socketutils::ShutdownWrite(socket_fd_);
    }
    return;
  }
  int writen_bytes = socketutils::Write(socket_fd_,
                                        out_buffer_.GetRead(),
                                        data_size);

  if (writen_bytes > 0) {
    out_buffer_.Consume(writen_bytes);

    if (out_buffer_.CanReadSize() == 0) { //all data writen
      if (finish_write_callback_) {
        finish_write_callback_(shared_from_this());
      }
      if (proto_service_) {
        proto_service_->OnDataFinishSend(shared_from_this());
      }
    }
  } else {
    LOG(ERROR) << "Call Socket Write Error";
  }

  if (out_buffer_.CanReadSize() == 0) { //all data writen
    fd_event_->DisableWriting();

    if (schedule_shutdown_) {
      socketutils::ShutdownWrite(socket_fd_);
    }
  }
}

bool TcpChannel::SendProtoMessage(RefProtocolMessage message) {
  CHECK(work_loop_->IsInLoopThread());
  CHECK(proto_service_);

  if (!message) {
    VLOG(GLOG_VERROR) << "Bad ProtoMessage, ChannelInfo:" << ChannelName() << " Status:" << StatusAsString();
    return false;
  }
  if (channel_status_ != ChannelStatus::CONNECTED) {
    VLOG(GLOG_VERROR) << "Channel Is Broken, ChannelInfo:" << ChannelName() << " Status:" << StatusAsString();
    return false;
  }

  if (out_buffer_.CanReadSize()) { //append to out_buffer_

    bool success = proto_service_->EncodeToBuffer(message.get(), &out_buffer_);
    LOG_IF(ERROR, !success) << "Send Failed For Message Encode Failed";

    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }

  } else { //out_buffer_ empty, try write directly

    IOBuffer buffer;
    bool success = proto_service_->EncodeToBuffer(message.get(), &buffer);
    LOG_IF(ERROR, !success) << "Send Failed For Message Encode Failed";
    if (success && buffer.CanReadSize()) {
      Send(buffer.GetRead(), buffer.CanReadSize());
    }
  }
  return true;
}

void TcpChannel::HandleError() {
  int err = socketutils::GetSocketError(socket_fd_);
  thread_local static char t_err_buff[128];
  LOG(ERROR) << " Socket Error, fd:[" << socket_fd_
             << "], error info: [" << strerror_r(err, t_err_buff, sizeof t_err_buff)
             << "]";
}

void TcpChannel::HandleClose() {
  CHECK(work_loop_->IsInLoopThread());

  RefTcpChannel guard(shared_from_this());
  if (channel_status_ == DISCONNECTED) {
    return;
  }

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
  if (proto_service_) {
    proto_service_->OnStatusChanged(guard);
  }
}

void TcpChannel::ShutdownChannel() {
  CHECK(InIOLoop());

  VLOG(GLOG_VTRACE) << "TcpChannel::ShutdownChannel " << channal_name_;
  if (channel_status_ == DISCONNECTED) {
    return;
  }

  if (!fd_event_->IsWriteEnable()) {
    SetChannelStatus(DISCONNECTING);
    socketutils::ShutdownWrite(socket_fd_);
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
    n_write = socketutils::Write(socket_fd_, data, len);

    if (n_write >= 0) {

      n_remain = len - n_write;

      if (n_remain == 0) { //finish all
        if (proto_service_) {
          proto_service_->OnDataFinishSend(shared_from_this());
        }
        if (finish_write_callback_) {
          finish_write_callback_(shared_from_this());
        }
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

RefProtoService TcpChannel::GetProtoService() const {
  return proto_service_;
}

void TcpChannel::SetProtoService(RefProtoService proto_service) {
  proto_service_ = proto_service;
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
