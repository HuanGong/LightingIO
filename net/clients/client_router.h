#ifndef LT_NET_CLIENT_ROUTER_H_H
#define LT_NET_CLIENT_ROUTER_H_H

#include <vector>

#include "inet_address.h"
#include "tcp_channel.h"
#include "socket_utils.h"
#include "service_acceptor.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service_factory.h"

#include <cinttypes>
#include "clients/client_channel.h"
#include "clients/client_router.h"
#include "clients/client_connector.h"
#include "dispatcher/coro_dispatcher.h"

namespace net {

class ClientRouter : public ConnectorDelegate {
public:
  ClientRouter(base::MessageLoop2*, const InetAddress&);
  ~ClientRouter();

  bool StartRouter();
  void OnNewClientConnected(int fd, InetAddress& loc, InetAddress& remote) override;

private:
  void OnRequestGetResponse(RefProtocolMessage& req, RefProtocolMessage& res);
  void OnClientChannelClosed(RefClientChannel channel);

private:
  std::string protocol_;
  uint32_t channel_count_;
  uint32_t reconnect_interval_; //ms
  uint32_t connection_timeout_; //ms
  const InetAddress server_addr_;
  base::MessageLoop2* work_loop_;

  RefConnector connector_;
  std::vector<RefClientChannel> channels_;
};

}//end namespace net
#endif
