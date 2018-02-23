
#include "client_connector.h"

namespace net {

Connector::Connector(base::MessageLoop2* loop, ConnectorDelegate* delegate)
  : loop_(loop),
    delegate_(delegate) {
  CHECK(delegate_);
}

bool Connector::LaunchAConnection(net::InetAddress& address) {
  CHECK(loop_->IsInLoopThread());

  int sockfd = net::socketutils::CreateNonBlockingSocket(address.SocketFamily());
  const struct sockaddr* sock_addr = net::socketutils::sockaddr_cast(address.SockAddrIn());
  int ret = net::socketutils::SocketConnect(sockfd, sock_addr);
  int state = ret == 0 ? 0 : errno;
  switch(state)  {
    case 0:
    case EINTR:
    case EISCONN:
    case EINPROGRESS: {

      base::RefFdEvent fdevent = base::FdEvent::create(sockfd, 0);

      WeakPtrFdEvent weak_fdevent(fdevent);

      fdevent->SetWriteCallback(std::bind(&Connector::OnWrite, this, weak_fdevent));
      fdevent->SetErrorCallback(std::bind(&Connector::OnError, this, weak_fdevent));
      fdevent->SetCloseCallback(std::bind(&Connector::OnError, this, weak_fdevent));
      fdevent->SetDelegate(loop_->Pump()->AsFdEventDelegate());

      InitEvent(fdevent);
      connecting_sockets_.insert(fdevent);

      return true;

    } break;
    default:
      LOG(FATAL) << " FATAL ERROR";
      net::socketutils::CloseSocket(sockfd);
      return false;
    break;
  }
  return true;
}

void Connector::OnWrite(WeakPtrFdEvent weak_fdevent) {
  base::RefFdEvent fd_event = weak_fdevent.lock();
  if (!fd_event) {
    LOG(ERROR) << __FUNCTION__ << " FdEvent Has Gone!";
    return ;
  }

  int socket_fd = fd_event->fd();

  if (net::socketutils::IsSelfConnect(socket_fd)) {
    CleanUpBadChannel(fd_event);
    delegate_->OnClientConnectFailed();
    return;
  }

  loop_->Pump()->RemoveFdEvent(fd_event.get());
  connecting_sockets_.erase(fd_event);

  net::InetAddress remote_addr(net::socketutils::GetPeerAddrIn(socket_fd));
  net::InetAddress local_addr(net::socketutils::GetLocalAddrIn(socket_fd));

  delegate_->OnNewClientConnected(socket_fd, local_addr, remote_addr);
}

void Connector::OnError(WeakPtrFdEvent weak_fdevent) {
  base::RefFdEvent event = weak_fdevent.lock();
  if (!event) {
    return;
  }
  CleanUpBadChannel(event);
  delegate_->OnClientConnectFailed();
}

void Connector::DiscardAllConnectingClient() {
  CHECK(loop_->IsInLoopThread());
  for (base::RefFdEvent event : connecting_sockets_) {
    int socket_fd = event->fd();
    loop_->Pump()->RemoveFdEvent(event.get());
    net::socketutils::CloseSocket(socket_fd);
  }
  connecting_sockets_.clear();
}

void Connector::InitEvent(base::RefFdEvent& fd_event) {
  loop_->Pump()->InstallFdEvent(fd_event.get());
  fd_event->EnableWriting();
}

void Connector::CleanUpBadChannel(base::RefFdEvent& event) {
  int socket_fd = event->fd();
  loop_->Pump()->RemoveFdEvent(event.get());
  event->ResetCallback();
  net::socketutils::CloseSocket(socket_fd);
  connecting_sockets_.erase(event);

  LOG(INFO) << "Now Has " << connecting_sockets_.size() << " In Connecting Progress";
}

}//end namespace
