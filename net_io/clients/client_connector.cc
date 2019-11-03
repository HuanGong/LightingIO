#include "client_connector.h"
#include "message_loop/event.h"
#include <base/utils/sys_error.h>

namespace lt {
namespace net {

Connector::Connector(base::MessageLoop* loop, ConnectorDelegate* delegate)
  : loop_(loop),
    delegate_(delegate) {
  CHECK(delegate_);
}

bool Connector::Launch(const net::SocketAddr &address) {
  CHECK(loop_->IsInLoopThread());

  int sock_fd = net::socketutils::CreateNonBlockingSocket(address.Family());
  if (sock_fd == -1) {
    LOG(INFO) << __FUNCTION__ << " create none block socket failed";
    return false;
  }

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " connect to add:" << address.IpPort();
  const struct sockaddr* sock_addr = net::socketutils::sockaddr_cast(address.SockAddrIn());

  bool success = false;

  int state = 0;
  socketutils::SocketConnect(sock_fd, sock_addr, &state);

  switch(state) {
    case 0:
    case EINTR:
    case EISCONN:
    case EINPROGRESS: {

      success = true;
      base::RefFdEvent fd_event = base::FdEvent::Create(sock_fd, base::LtEv::LT_EVENT_NONE);

      WeakPtrFdEvent weak_fd_event(fd_event);
      fd_event->SetWriteCallback(std::bind(&Connector::OnWrite, this, weak_fd_event));
      fd_event->SetErrorCallback(std::bind(&Connector::OnError, this, weak_fd_event));
      fd_event->SetCloseCallback(std::bind(&Connector::OnError, this, weak_fd_event));
      loop_->Pump()->InstallFdEvent(fd_event.get());
      fd_event->EnableWriting();

      connecting_sockets_.insert(fd_event);
      VLOG(GLOG_VTRACE) << __FUNCTION__ << " " << address.IpPort() << " connecting";
    } break;
    default: {
      success = false;
      LOG(ERROR) << __FUNCTION__ << " launch client connect failed:" << base::StrError(state);
      net::socketutils::CloseSocket(sock_fd);
    } break;
  }
  if (!success && delegate_) {
    delegate_->OnClientConnectFailed();
  }
  return success;
}

void Connector::OnWrite(WeakPtrFdEvent weak_fdevent) {
  base::RefFdEvent fd_event = weak_fdevent.lock();

  if (!fd_event && delegate_) {
    delegate_->OnClientConnectFailed();
    LOG(ERROR) << __FUNCTION__ << " FdEvent Has Gone Before Connection Setup";
    return;
  }

  int socket_fd = fd_event->fd();

  if (net::socketutils::IsSelfConnect(socket_fd)) {
    CleanUpBadChannel(fd_event);
    if (delegate_) {
      delegate_->OnClientConnectFailed();
    }
    return;
  }

  loop_->Pump()->RemoveFdEvent(fd_event.get());
  connecting_sockets_.erase(fd_event);
  if (delegate_) {
    fd_event->GiveupOwnerFd();
    net::SocketAddr remote_addr(net::socketutils::GetPeerAddrIn(socket_fd));
    net::SocketAddr local_addr(net::socketutils::GetLocalAddrIn(socket_fd));
    delegate_->OnNewClientConnected(socket_fd, local_addr, remote_addr);
  }
}

void Connector::OnError(WeakPtrFdEvent weak_fdevent) {
  base::RefFdEvent event = weak_fdevent.lock();
  if (!event && delegate_) {
    delegate_->OnClientConnectFailed();
    return;
  }

  CleanUpBadChannel(event);
  if (delegate_) {
    delegate_->OnClientConnectFailed();
  }
}

void Connector::Stop() {
  CHECK(loop_->IsInLoopThread());
  delegate_ = NULL;
  for (base::RefFdEvent event : connecting_sockets_) {
    int socket_fd = event->fd();
    loop_->Pump()->RemoveFdEvent(event.get());
    net::socketutils::CloseSocket(socket_fd);
  }
  connecting_sockets_.clear();
}

void Connector::CleanUpBadChannel(base::RefFdEvent& event) {

  event->ResetCallback();
  loop_->Pump()->RemoveFdEvent(event.get());

  connecting_sockets_.erase(event);
  VLOG(GLOG_VINFO) << " connecting list size:" << connecting_sockets_.size();
}

}}//end namespace
