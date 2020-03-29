#ifndef LT_NET_CLIENT_H_H
#define LT_NET_CLIENT_H_H

#include <vector>
#include <memory>
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <cinttypes>
#include <initializer_list>
#include <condition_variable> // std::condition_variable, std::cv_status

#include "client_base.h"
#include "queued_channel.h"
#include "client_connector.h"

#include <net_io/url_utils.h>
#include "net_io/address.h"
#include "net_io/tcp_channel.h"
#include "net_io/socket_utils.h"
#include "net_io/socket_acceptor.h"
#include "net_io/protocol/proto_service.h"
#include "net_io/protocol/proto_message.h"
#include "net_io/protocol/proto_service_factory.h"

namespace lt {
namespace net {

class Client;
typedef std::shared_ptr<Client> RefClient;
typedef std::function<void(ProtocolMessage*)> AsyncCallBack;

class ClientDelegate {
public:
  virtual base::MessageLoop* NextIOLoopForClient() = 0;
  /* do some init things like select db for redis,
   * or enable keepalive action etc*/
  virtual void OnClientChannelReady(const ClientChannel* channel) {}
  /* when a abnormal broken happend, here a chance to notify
   * application level for handle this case, eg:remote server
   * shutdown,crash etc*/
  virtual void OnAllClientPassiveBroken(const Client* client) {};
};

class Client: public ConnectorDelegate,
              public ClientChannel::Delegate {
public:
  class Interceptor {
    public:
      Interceptor() {};
      virtual ~Interceptor() {};
    public:
      //if has a response, return a response without call next, else call next for continue
      virtual ProtocolMessage* Intercept(const Client* c, const ProtocolMessage* req) = 0;
  };
  typedef std::initializer_list<Interceptor*> InterceptArgList;

public:
  Client(base::MessageLoop*, const url::RemoteInfo&);
  virtual ~Client();

  void Finalize();
  void Initialize(const ClientConfig& config);
  void SetDelegate(ClientDelegate* delegate);
  Client* Use(Interceptor* interceptor);
  Client* Use(InterceptArgList interceptors);

  template<class T>
  typename T::element_type::ResponseType* SendRecieve(T& m) {
    RefProtocolMessage message = std::static_pointer_cast<ProtocolMessage>(m);
    return (typename T::element_type::ResponseType*)(SendClientRequest(message));
  }
  ProtocolMessage* SendClientRequest(RefProtocolMessage& message);

  bool AsyncSendRequest(RefProtocolMessage& req, AsyncCallBack);

  // notified from connector
  void OnClientConnectFailed() override;
  // notified from connector
  void OnNewClientConnected(int fd, SocketAddr& loc, SocketAddr& remote) override;

  // clientchanneldelegate
  const ClientConfig& GetClientConfig() const override {return config_;}
  const url::RemoteInfo& GetRemoteInfo() const override {return remote_info_;}
  void OnClientChannelInited(const ClientChannel* channel) override;
  //only called when passive close, active close won't be notified for thread-safe reason
  void OnClientChannelClosed(const RefClientChannel& channel) override;
  void OnRequestGetResponse(const RefProtocolMessage&, const RefProtocolMessage&) override;

  uint64_t ConnectedCount() const;
  std::string ClientInfo() const;
  std::string RemoteIpPort() const;
private:
  /*return a connected and initialized channel*/
  RefClientChannel get_ready_channel();

  /*return a io loop for client channel work on*/
  base::MessageLoop* next_client_io_loop();

  SocketAddr address_;
  const url::RemoteInfo remote_info_;

  /* workloop mean: all client channel born & die, not mean
   * clientchannel io_loop, client channel may work in other loop*/
  base::MessageLoop* work_loop_;

  std::atomic<bool> stopping_;

  ClientConfig config_;
  RefConnector connector_;
  ClientDelegate* delegate_;
  typedef std::vector<RefClientChannel> ClientChannelList;
  typedef std::shared_ptr<ClientChannelList> RefClientChannelList;

  //a channels copy for client caller
  std::atomic<uint32_t> next_index_;
  RefClientChannelList in_use_channels_;
  std::vector<Interceptor*> interceptors_;
  DISALLOW_COPY_AND_ASSIGN(Client);
};

}}//end namespace lt::net
#endif
