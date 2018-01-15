#ifndef _LT_NET_CLIENT_CONNECTOR_H_H
#define _LT_NET_CLIENT_CONNECTOR_H_H

#include <set>

#include "../tcp_channel.h"
#include "../socket_utils.h"
#include "../service_acceptor.h"
#include "../protocol/proto_service.h"
#include "../protocol/line/line_message.h"
#include "../protocol/http/http_request.h"
#include "../protocol/http/http_response.h"
#include "../protocol/proto_service_factory.h"

namespace net {

typedef std::weak_ptr<base::FdEvent> WeakPtrFdEvent;

class ConnectorDelegate {
public:
  virtual ~ConnectorDelegate() {};
  virtual void OnNewClientConnected(int socket_fd, InetAddress& local, InetAddress& remote) = 0;
};

class Connector {
public:
  Connector(base::MessageLoop2* loop, ConnectorDelegate* delegate);
  ~Connector() {};

  bool LaunchAConnection(net::InetAddress& address);

  void OnWrite(WeakPtrFdEvent weak_fdevent);
  void OnError(WeakPtrFdEvent weak_fdevent);

  void DiscardAllConnectingClient();
  void OnConnectTimeout(base::RefFdEvent);

private:
  void InitEvent(base::RefFdEvent& fd_event);
  void CleanUpBadChannel(base::RefFdEvent& event);

private:
  base::MessageLoop2* loop_;
  ConnectorDelegate* delegate_;
  std::set<base::RefFdEvent> connecting_sockets_;
};

}
#endif
