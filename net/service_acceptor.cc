
#include "service_acceptor.h"
#include "base/base_constants.h"

#include "glog/logging.h"

namespace net {

ServiceAcceptor::ServiceAcceptor(base::MessageLoop2* loop, const InetAddress& address)
  : listenning_(false),
    owner_loop_(loop),
    address_(address) {

  CHECK(owner_loop_->IsInLoopThread());

  socket_fd_ = socketutils::CreateNonBlockingSocket(address_.SocketFamily());
  //reuse socket addr and port if possible
  socketutils::ReUseSocketAddress(socket_fd_, true);
  socketutils::ReUseSocketPort(socket_fd_, true);
  socketutils::BindSocketFd(socket_fd_, address_.AsSocketAddr());

  socket_event_ = base::FdEvent::create(socket_fd_, 0);
  socket_event_->SetDelegate(owner_loop_->Pump()->AsFdEventDelegate());
  socket_event_->SetReadCallback(
    std::bind(&ServiceAcceptor::HandleCommingConnection, this));

  //socket_event_->EnableReading();
}

ServiceAcceptor::~ServiceAcceptor() {

  CHECK(owner_loop_->IsInLoopThread());

  socket_event_->DisableAll();
  owner_loop_->Pump()->RemoveFdEvent(socket_event_.get());
  socket_event_.reset();
  socketutils::CloseSocket(socket_fd_);
}

bool ServiceAcceptor::StartListen() {
  CHECK(owner_loop_->IsInLoopThread());

  owner_loop_->Pump()->InstallFdEvent(socket_event_.get());
  socket_event_->EnableReading();

  listenning_ = true;
  socketutils::ListenSocket(socket_fd_);
  LOG(INFO) << " Start Listen on:" << address_.IpPortAsString();
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
