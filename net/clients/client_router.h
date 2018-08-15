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

#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status
#include <cinttypes>
#include "clients/client_channel.h"
#include "clients/client_router.h"
#include "clients/client_connector.h"
#include "dispatcher/coro_dispatcher.h"

namespace net {

typedef struct {
  std::string protocol;
  uint32_t connections;
  uint32_t recon_interal;
  uint32_t message_timeout;
} RouterConf;

class RouterDelegate {
public:
  virtual base::MessageLoop* NextIOLoopForClient() = 0;
};

class ClientRouter;
typedef std::shared_ptr<ClientRouter> RefClientRouter;

class ClientRouter : public ConnectorDelegate,
                     public ClientChannel::Delegate {
public:
  ClientRouter(base::MessageLoop*, const InetAddress&);
  ~ClientRouter();

  void SetDelegate(RouterDelegate* delegate);
  void SetupRouter(const RouterConf& config);
  void SetWorkLoadTransfer(CoroWlDispatcher* dispatcher);

  void StartRouter();
  void StopRouter();
  void StopRouterSync();
  bool SendClientRequest(RefProtocolMessage& message);

  template<class T>
  bool SendRecieve(T& message) {
    RefProtocolMessage m = std::static_pointer_cast<ProtocolMessage>(message);
    return SendClientRequest(m);
  }

  //override from ConnectorDelegate
  void OnClientConnectFailed() override;
  void OnNewClientConnected(int fd, InetAddress& loc, InetAddress& remote) override;

  //override from ClientChannel::Delegate
  void OnClientChannelClosed(RefClientChannel channel) override;
  void OnRequestGetResponse(RefProtocolMessage, RefProtocolMessage) override;

private:
  //Get a io work loop for channel, if no loop provide, use default work_loop_;
  base::MessageLoop* GetLoopForClient();
  void SendRequestInWorkLoop(RefProtocolMessage message);

  std::string protocol_;
  uint32_t channel_count_;
  uint32_t reconnect_interval_; //ms
  uint32_t connection_timeout_; //ms

  uint32_t message_timeout_; //ms

  const InetAddress server_addr_;
  base::MessageLoop* work_loop_;

  bool is_stopping_;
  std::mutex mtx_;
  std::condition_variable cv_;

  RefConnector connector_;
  RouterDelegate* delegate_;
  CoroWlDispatcher* dispatcher_;

  std::vector<RefClientChannel> channels_;
  std::atomic<uint32_t> router_counter_;
};

}//end namespace net
#endif
