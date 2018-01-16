#include "client_router.h"

namespace net {

ClientRouter::ClientRouter(base::MessageLoop2* loop, const InetAddress& server)
  : protocol_("http"),
    channel_count_(1),
    reconnect_interval_(2000),
    connection_timeout_(5000),
    server_addr_(server),
    work_loop_(loop) {

  CHECK(work_loop_);

  connector_ = std::make_shared<Connector>(work_loop_, this);
}

ClientRouter::~ClientRouter() {
}


bool ClientRouter::StartRouter() {

}

void ClientRouter::OnNewClientConnected(int socket_fd, InetAddress& local, InetAddress& remote) {
  LOG(ERROR) << " OnChannelConnected";
  //try get a io work loop for channel, if no loop provide, use default work_loop_;

  RefTcpChannel new_channel = TcpChannel::CreateClientChannel(socket_fd,
                                                              local,
                                                              remote,
                                                              work_loop_);
  new_channel->SetOwnerLoop(work_loop_);
  //Is peer_addr.IpPortAsString Is unique?
  new_channel->SetChannelName(remote.IpPortAsString());

  RefClientChannel client_channel = std::make_shared<ClientChannel>(new_channel);
  client_channel->SetResponseHandler(std::bind(&ClientRouter::OnRequestGetResponse,
                                               this,
                                               std::placeholders::_1,
                                               std::placeholders::_2));

  RefProtoService proto_service = ProtoServiceFactory::Instance().Create(protocol_);
  //proto_service->SetMessageDispatcher(g_dispatcher);
  proto_service->SetMessageHandler(std::bind(&ClientChannel::OnResponseMessage,
                                             client_channel,
                                             std::placeholders::_1));
  new_channel->SetProtoService(proto_service);

  channels_.push_back(client_channel);
}

void ClientRouter::OnClientChannelClosed(RefClientChannel channel) {
  CHECK(work_loop_->IsInLoopThread());

  auto iter = std::find(channels_.begin(), channels_.end(), channel);
  if (iter != channels_.end()) {
    channels_.erase(iter);
  } else {
    LOG(ERROR) << "Not Should Happend, What Happend!";
  }
}

void ClientRouter::OnRequestGetResponse(RefProtocolMessage& request,
                                        RefProtocolMessage& response) {
  LOG(INFO) << " Request Got A Resonse ";
  request->SetResponse(response);
  //dispatcher->DiaptchResponse();
  //dispatcher->DiaptchResponse();
}

}

