

#include "service_acceptor.h"
#include "base/base_constants.h"

#include "glog/logging.h"

namespace net {

ServiceAcceptor::ServiceAcceptor(base::EventPump* pump, const InetAddress& address)
  : listenning_(false),
    event_pump_(pump),
    address_(address) {
  CHECK(event_pump_);

  InitListener();
}

ServiceAcceptor::~ServiceAcceptor() {
  CHECK(listenning_ == false);

  socket_event_.reset();
}

void ServiceAcceptor::InitListener() {
  int socket_fd = socketutils::CreateNonBlockingSocket(address_.SocketFamily());
  //reuse socket addr and port if possible
  socketutils::ReUseSocketAddress(socket_fd, true);
  socketutils::ReUseSocketPort(socket_fd, true);
  socketutils::BindSocketFd(socket_fd, address_.AsSocketAddr());

  socket_event_ = base::FdEvent::Create(socket_fd, 0);
  socket_event_->SetCloseCallback(std::bind(&ServiceAcceptor::OnAccepterError, this));
  socket_event_->SetErrorCallback(std::bind(&ServiceAcceptor::OnAccepterError, this));
  socket_event_->SetReadCallback(std::bind(&ServiceAcceptor::HandleCommingConnection, this));

  VLOG(GLOG_VTRACE) << " Server Accept Socket Fd:" << socket_fd;
}

bool ServiceAcceptor::StartListen() {
  CHECK(event_pump_->IsInLoopThread());

  if (listenning_) {
    LOG(ERROR) << " Aready Listen on:" << address_.IpPortAsString();
    return true;
  }

  event_pump_->InstallFdEvent(socket_event_.get());
  socket_event_->EnableReading();

  listenning_ = true;
  socketutils::ListenSocket(socket_event_->fd());
  LOG(INFO) << " Start Listen on:" << address_.IpPortAsString();
  return true;
}

void ServiceAcceptor::StopListen() {
  CHECK(event_pump_->IsInLoopThread());

  if (!listenning_) {
    return;
  }

  socket_event_->DisableAll();
  event_pump_->RemoveFdEvent(socket_event_.get());
  listenning_ = false;
  LOG(INFO) << " Stop Listen on:" << address_.IpPortAsString();
}

void ServiceAcceptor::SetNewConnectionCallback(const NewConnectionCallback& cb) {
  new_conn_callback_ = std::move(cb);
}

void ServiceAcceptor::HandleCommingConnection() {

  struct sockaddr_in client_socket_in;
  int peer_fd = socketutils::AcceptSocket(socket_event_->fd(), &client_socket_in);

  if (peer_fd <= 0) {
    LOG(ERROR) << "AcceptSocket Failed";
    return ;
  }

  InetAddress clientaddr(client_socket_in);

  VLOG(GLOG_VTRACE) << "Accept a New Connection:" << clientaddr.IpPortAsString();

  if (new_conn_callback_) {
    new_conn_callback_(peer_fd, clientaddr);
  } else {
    socketutils::CloseSocket(peer_fd);
  }
}

void ServiceAcceptor::OnAccepterError() {
  CHECK(event_pump_->IsInLoopThread());

  LOG(ERROR) << "Acceptor" << socket_event_->fd() << " Occur Error, Relaunch It";
  // Relaunch This server 
  InitListener();
  StartListen();
}

}//end namespace net
