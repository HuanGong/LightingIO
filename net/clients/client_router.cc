#include "client_router.h"
#include "base/coroutine/coroutine_scheduler.h"

namespace net {

ClientRouter::ClientRouter(base::MessageLoop2* loop, const InetAddress& server)
  : protocol_("http"),
    channel_count_(1),
    reconnect_interval_(2000),
    connection_timeout_(5000),
    server_addr_(server),
    work_loop_(loop) {
  router_counter_ = 0;
  CHECK(work_loop_);
  connector_ = std::make_shared<Connector>(work_loop_, this);
}

ClientRouter::~ClientRouter() {
}


void ClientRouter::StartRouter() {
  for (uint32_t i = 0; i < channel_count_; i++) {
    work_loop_->PostTask(base::NewClosure(std::bind(&Connector::LaunchAConnection, connector_, server_addr_)));
  }
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

  RefClientChannel client_channel = std::make_shared<ClientChannel>(this, new_channel);

  RefProtoService proto_service = ProtoServiceFactory::Instance().Create(protocol_);
  proto_service->SetMessageHandler(std::bind(&ClientChannel::OnResponseMessage,
                                             client_channel,
                                             std::placeholders::_1));
  new_channel->SetProtoService(proto_service);

  channels_.push_back(client_channel);
}

void ClientRouter::OnClientChannelClosed(RefClientChannel channel) {
  LOG(ERROR) << " OnChannelConnected";
  if (!work_loop_->IsInLoopThread()) {
    work_loop_->PostTask(base::NewClosure(std::bind(&ClientRouter::OnClientChannelClosed, this, channel)));
    return;
  }

  auto iter = std::find(channels_.begin(), channels_.end(), channel);
  if (iter != channels_.end()) {
    channels_.erase(iter);
  } else {
    LOG(ERROR) << "Not Should Happend, What Happend!";
  }
}

void ClientRouter::OnRequestGetResponse(RefProtocolMessage request,
                                        RefProtocolMessage response) {
  if (response) {
    LOG(INFO) << " Request Got A Resonse " << response->MessageDebug();
  }
  request->SetResponse(response);

  auto& work_context = request->GetWorkCtx();
  work_context.coro_loop->PostTask(base::NewClosure([=]() {
    base::RefCoroutine coro = work_context.weak_coro.lock();
    if (coro) {
      base::CoroScheduler::TlsCurrent()->ResumeCoroutine(coro->Identifier());
    }
  }));
}

bool ClientRouter::SendClientRequest(RefProtocolMessage& message) {
  if (0 == channels_.size()) {
    LOG(ERROR) << " No ClientChannel Established, clients count:" << channels_.size()
               << " Server:" << server_addr_.IpPortAsString();
    return false;
  }
  uint32_t index = router_counter_ % channels_.size();
  RefClientChannel& client = channels_[index];
  base::MessageLoop2* io_loop = client->IOLoop();
  if (NULL == io_loop) {
    return false;
  }
  router_counter_++;

  if (!base::MessageLoop2::Current()) {
    return false;
  }
  if (!base::CoroScheduler::TlsCurrent()->CurrentCoro()) {
    return false;
  }
  auto& work_context = message->GetWorkCtx();
  work_context.coro_loop = base::MessageLoop2::Current();
  work_context.weak_coro = base::CoroScheduler::TlsCurrent()->CurrentCoro();

  io_loop->PostTask(base::NewClosure(std::bind(&ClientChannel::ScheduleARequest, client, message)));
  return true;
}

}

