

#include "service_acceptor.h"
#include "base/base_constants.h"

#include "glog/logging.h"

namespace net {

ServiceAcceptor::ServiceAcceptor(base::EventPump* pump, const InetAddress& address)
  : listenning_(false),
    event_pump_(pump),
    socket_fd_(0),
    address_(address) {
  CHECK(event_pump_);
  //CHECK(event_pump_->IsInLoopThread());
  InitListener();
  //socket_event_->EnableReading();
}

ServiceAcceptor::~ServiceAcceptor() {
  //CHECK(event_pump_->IsInLoopThread());
  if (socket_fd_) {
    socketutils::CloseSocket(socket_fd_);
  }
}

void ServiceAcceptor::InitListener() {
  socket_fd_ = socketutils::CreateNonBlockingSocket(address_.SocketFamily());
  //reuse socket addr and port if possible
  socketutils::ReUseSocketAddress(socket_fd_, true);
  socketutils::ReUseSocketPort(socket_fd_, true);
  socketutils::BindSocketFd(socket_fd_, address_.AsSocketAddr());

  socket_event_ = base::FdEvent::create(socket_fd_, 0);
  socket_event_->SetDelegate(event_pump_->AsFdEventDelegate());
  socket_event_->SetReadCallback(
    std::bind(&ServiceAcceptor::HandleCommingConnection, this));
}

bool ServiceAcceptor::StartListen() {
  CHECK(event_pump_->IsInLoopThread());
  if (listenning_) {
    LOG(INFO) << " Aready Listen on:" << address_.IpPortAsString();
    return true;
  }
  if (!socket_event_) {
    InitListener();
  }

  event_pump_->InstallFdEvent(socket_event_.get());
  socket_event_->EnableReading();

  listenning_ = true;
  socketutils::ListenSocket(socket_fd_);
  LOG(INFO) << " Start Listen on:" << address_.IpPortAsString();
}

void ServiceAcceptor::StopListen() {
  CHECK(event_pump_->IsInLoopThread());
  if (!listenning_) {
    return;
  }

  socket_event_->DisableReading();
  event_pump_->RemoveFdEvent(socket_event_.get());
  socket_event_.reset();
  socketutils::CloseSocket(socket_fd_);
  socket_fd_ = 0;
  LOG(INFO) << " Stop Listen on:" << address_.IpPortAsString();
}

void ServiceAcceptor::SetNewConnectionCallback(const NewConnectionCallback& cb) {
  new_conn_callback_ = std::move(cb);
}

void ServiceAcceptor::HandleCommingConnection() {

  VLOG(GLOG_VINFO) << "Accept a New Connection";

  struct sockaddr_in client_socket_in;
  int peer_fd = socketutils::AcceptSocket(socket_fd_, &client_socket_in);

  if (peer_fd <= 0) {
    LOG(ERROR) << "AcceptSocket Failed";
    return ;
  }

  InetAddress clientaddr(client_socket_in);
  if (new_conn_callback_) {
    new_conn_callback_(peer_fd, clientaddr);
  } else {
    socketutils::CloseSocket(peer_fd);
  }
}

}//end namespace net
