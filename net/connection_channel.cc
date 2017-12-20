#include "connection_channel.h"
#include "base/closure/closure_task.h"

#include "base/base_constants.h"
#include "glog/logging.h"

namespace net {

void DefaultCloseHandle(const RefConnectionChannel& conn) {

}

//static
RefConnectionChannel ConnectionChannel::Create(int socket_fd,
                                               const InetAddress& local,
                                               const InetAddress& peer,
                                               base::MessageLoop2* loop) {

  RefConnectionChannel conn(new ConnectionChannel(socket_fd,
                                                  local,
                                                  peer,
                                                  loop));
  conn->Initialize();
  return std::move(conn);
}

ConnectionChannel::ConnectionChannel(int socket_fd,
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

void ConnectionChannel::Initialize() {
  //conside move follow to InitConnection ensure enable_shared_from_this work
  base::EventPump* event_pump = work_loop_->Pump();
  fd_event_->SetDelegate(event_pump->AsFdEventDelegate());
  fd_event_->SetReadCallback(std::bind(&ConnectionChannel::HandleRead, this));
  fd_event_->SetWriteCallback(std::bind(&ConnectionChannel::HandleWrite, this));
  fd_event_->SetCloseCallback(std::bind(&ConnectionChannel::HandleClose, this));
  fd_event_->SetErrorCallback(std::bind(&ConnectionChannel::HandleError, this));

  socketutils::KeepAlive(socket_fd_, true);

  auto task = base::NewClosure(std::bind(&ConnectionChannel::OnConnectionReady,
                                         shared_from_this()));
  work_loop_->PostTask(std::move(task));
}

ConnectionChannel::~ConnectionChannel() {
  VLOG(GLOG_VTRACE) << "ConnectionChannel Gone, Fd:" << socket_fd_ << " status:" << StatusToString();
  //CHECK(channel_status_ == DISCONNECTED);
  socketutils::CloseSocket(socket_fd_);
}

void ConnectionChannel::OnConnectionReady() {
  if (channel_status_ == CONNECTING) {

    base::EventPump* event_pump = work_loop_->Pump();
    event_pump->InstallFdEvent(fd_event_.get());
    fd_event_->EnableReading();
    //fd_event_->EnableWriting();

    channel_status_ = CONNECTED;
    OnStatusChanged();
  } else {
    LOG(INFO) << "This Connection Status Changed After Constructor";
  }
}

void ConnectionChannel::OnStatusChanged() {
  if (status_change_callback_) {
    RefConnectionChannel guard(shared_from_this());
    status_change_callback_(guard);
  }
}

void ConnectionChannel::HandleRead() {
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

void ConnectionChannel::HandleWrite() {

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

void ConnectionChannel::HandleError() {
  int err = socketutils::GetSocketError(socket_fd_);
  thread_local static char t_err_buff[128];
  LOG(ERROR) << " Socket Error, fd:[" << socket_fd_ << "], error info: [" << strerror_r(err, t_err_buff, sizeof t_err_buff) << " ]";
}

void ConnectionChannel::HandleClose() {

  CHECK(work_loop_->IsInLoopThread());
  if (channel_status_ == DISCONNECTED) {
    return;
  }

  channel_status_ = DISCONNECTED;

  fd_event_->DisableAll();
  work_loop_->Pump()->RemoveFdEvent(fd_event_.get());

  RefConnectionChannel guard(shared_from_this());

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

std::string ConnectionChannel::StatusToString() const {
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

void ConnectionChannel::ShutdownConnection() {
  if (!work_loop_->IsInLoopThread()) {
    work_loop_->PostTask(base::NewClosure(
        std::bind(&ConnectionChannel::ShutdownConnection, shared_from_this())));
    return;
  }
  if (!fd_event_->IsWriteEnable()) {
    socketutils::ShutdownWrite(socket_fd_);
  }
}

int32_t ConnectionChannel::Send(const char* data, const int32_t len) {
  CHECK(work_loop_->IsInLoopThread());

  //if (channel_status_ != CONNECTED) {
    //LOG(INFO) <<  "Can't Write Data To a Closed[ing] socket";
    //return -1;
  //}

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

void ConnectionChannel::SetChannalName(const std::string name) {
  channal_name_ = name;
}
void ConnectionChannel::SetOwnerLoop(base::MessageLoop2* owner) {
  owner_loop_ = owner;
}
void ConnectionChannel::SetDataHandleCallback(const DataRcvCallback& callback) {
  recv_data_callback_ = callback;
}
void ConnectionChannel::SetFinishSendCallback(const DataWritenCallback& callback) {
  finish_write_callback_ = callback;
}
void ConnectionChannel::SetConnectionCloseCallback(const ConnectionClosedCallback& callback) {
  closed_callback_ = callback;
}
void ConnectionChannel::SetStatusChangedCallback(const ConnectionStatusCallback& callback) {
  status_change_callback_ = callback;
}

}
