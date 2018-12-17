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
#include <memory>
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status
#include <cinttypes>
#include "clients/queued_channel.h"
#include "clients/client_router.h"
#include "clients/client_connector.h"
#include "dispatcher/coro_dispatcher.h"

namespace net {

typedef struct {
  std::string protocol;
  uint32_t connections;
  uint32_t recon_interval;
  uint32_t message_timeout;
  //heart beat_ms only work for protocol service supported
  //  >0: mean keep heart beat every ms
  //zero: mean never need heart beat keep
  uint32_t heart_beat_ms;
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

  template<class T>
  typename T::element_type::ResponseType* SendRecieve(T& m) {
    RefProtocolMessage message = std::static_pointer_cast<ProtocolMessage>(m);
    return (typename T::element_type::ResponseType*)(SendClientRequest(message));
  }
  ProtocolMessage* SendClientRequest(RefProtocolMessage& message);

  //override from ConnectorDelegate
  void OnClientConnectFailed() override;
  void OnNewClientConnected(int fd, InetAddress& loc, InetAddress& remote) override;

  //override from ClientChannel::Delegate
  void OnClientChannelClosed(const RefClientChannel& channel) override;
  void OnRequestGetResponse(const RefProtocolMessage&, const RefProtocolMessage&) override;

  uint32_t ClientCount() const;
  std::string RouterInfo() const;
private:
  //Get a io work loop for channel, if no loop provide, use default io_loop_;
  base::MessageLoop* GetLoopForClient();

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
  typedef std::vector<RefClientChannel> ClientChannelList;

  ClientChannelList channels_;
  std::atomic<uint32_t> router_counter_;
  std::shared_ptr<ClientChannelList> roundrobin_channes_;
};

}//end namespace net
#endif
