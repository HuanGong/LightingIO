

#include "glog/logging.h"
#include "service_acceptor.h"
#include "base/base_constants.h"
#include "base/utils/sys_error.h"

namespace net {

ServiceAcceptor::ServiceAcceptor(base::EventPump* pump, const InetAddress& address)
  : listenning_(false),
    address_(address),
    event_pump_(pump) {
  CHECK(event_pump_);
  CHECK(InitListener());
}

ServiceAcceptor::~ServiceAcceptor() {
  CHECK(listenning_ == false);
  socket_event_.reset();
}

bool ServiceAcceptor::InitListener() {
  int socket_fd = socketutils::CreateNonBlockingSocket(address_.SocketFamily());
  if (socket_fd < 0) {
    return false;
  }
  //reuse socket addr and port if possible
  socketutils::ReUseSocketPort(socket_fd, true);
  socketutils::ReUseSocketAddress(socket_fd, true);
  int ret = socketutils::BindSocketFd(socket_fd, address_.AsSocketAddr());
  if (ret < 0) {
    socketutils::CloseSocket(socket_fd);
    return false;
  }
  socket_event_ = base::FdEvent::Create(socket_fd, 0);
  socket_event_->SetCloseCallback(std::bind(&ServiceAcceptor::OnAcceptorError, this));
  socket_event_->SetErrorCallback(std::bind(&ServiceAcceptor::OnAcceptorError, this));
  socket_event_->SetReadCallback(std::bind(&ServiceAcceptor::HandleCommingConnection, this));

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " init acceptor fd event success, fd:[" << socket_fd
                    << "] bind to local:[" << address_.IpPortAsString() << "]";
}

bool ServiceAcceptor::StartListen() {
  CHECK(event_pump_->IsInLoopThread());

  if (listenning_) {
    LOG(ERROR) << " Aready Listen on:" << address_.IpPortAsString();
    return true;
  }

  event_pump_->InstallFdEvent(socket_event_.get());
  socket_event_->EnableReading();

  listenning_ = socketutils::ListenSocket(socket_event_->fd()) == 0;
  LOG_IF(INFO, listenning_) << __FUNCTION__ << " satrt listen on:" << address_.IpPortAsString();
  return listenning_;
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

  int err = 0;
  int peer_fd = socketutils::AcceptSocket(socket_event_->fd(), &client_socket_in, &err);
  if (peer_fd < 0) {
    LOG(ERROR) << __FUNCTION__ << " accept new connection failed, err:" << base::StrError(err);
    return ;
  }

  InetAddress client_addr(client_socket_in);

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " accept a connection:" << client_addr.IpPortAsString();
  if (new_conn_callback_) {
    new_conn_callback_(peer_fd, client_addr);
  } else {
    socketutils::CloseSocket(peer_fd);
  }
}

void ServiceAcceptor::OnAcceptorError() {
  LOG(ERROR) << __FUNCTION__ << " accept fd [" << socket_event_->fd() << "] error:[" << base::StrError() << "]";

  listenning_ = false;

  // Relaunch This server 
  if (InitListener()) {
    bool re_listen_ok = StartListen();
    LOG_IF(ERROR, !re_listen_ok) << __FUNCTION__ << " acceptor:[" << address_.IpPortAsString() << "] re-listen failed";
  }
}

}//end namespace net
