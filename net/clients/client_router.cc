#include "client_router.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_scheduler.h"

namespace net {

ClientRouter::ClientRouter(base::MessageLoop2* loop, const InetAddress& server)
  : protocol_("http"),
    channel_count_(1),
    reconnect_interval_(2000),
    connection_timeout_(5000),
    server_addr_(server),
    work_loop_(loop),
    is_stopping_(false),
    delegate_(NULL),
    dispatcher_(NULL) {

  router_counter_ = 0;
  CHECK(work_loop_);
  connector_ = std::make_shared<Connector>(work_loop_, this);
}

ClientRouter::~ClientRouter() {

}

void ClientRouter::SetDelegate(RouterDelegate* delegate) {
  delegate_ = delegate;
}

void ClientRouter::SetupRouter(const RouterConf& config) {
  protocol_ = config.protocol;
  channel_count_ = config.connections;
  reconnect_interval_ = config.recon_interal;
  connection_timeout_ = config.message_timeout;
}

void ClientRouter::StartRouter() {
  for (uint32_t i = 0; i < channel_count_; i++) {
    work_loop_->PostTask(base::NewClosure(std::bind(&Connector::LaunchAConnection, connector_, server_addr_)));
  }
}

void ClientRouter::StopRouter() {
  if (is_stopping_ || 0 == channels_.size()) {
    return;
  }
  is_stopping_ = true;

  for (auto& client : channels_) {
    client->CloseClientChannel();
  }
}

void ClientRouter::OnClientConnectFailed() {
  if (is_stopping_) {
    return;
  }

  auto functor = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
  work_loop_->PostDelayTask(base::NewClosure(functor), reconnect_interval_);
}


base::MessageLoop2* ClientRouter::GetLoopForClient() {
  if (delegate_ == NULL) {
    base::MessageLoop2* io_loop = delegate_->NextIOLoopForClient();
    if (io_loop) {
      return io_loop;
    }
  }
  return work_loop_;
}

void ClientRouter::OnNewClientConnected(int socket_fd, InetAddress& local, InetAddress& remote) {
  LOG(ERROR) << " OnChannelConnected";

  //try get a io work loop for channel, if no loop provide, use default work_loop_;
  base::MessageLoop2* io_loop = GetLoopForClient();

  RefTcpChannel new_channel = TcpChannel::CreateClientChannel(socket_fd,
                                                              local,
                                                              remote,
                                                              io_loop);
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
  }

  //reconnect
  VLOG(GLOG_VTRACE) << " A Client Channel Broken, RefConnect After: " << reconnect_interval_ << " ms";

  if (!is_stopping_ && channels_.size() < channel_count_) {
    auto functor = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
    work_loop_->PostDelayTask(base::NewClosure(functor), reconnect_interval_);
  }
}

void ClientRouter::OnRequestGetResponse(RefProtocolMessage request,
                                        RefProtocolMessage response) {
  CHECK(request);

  if (response) {
    LOG(INFO) << " Request Got A Resonse " << response->MessageDebug();
  }

  request->SetResponse(response);

  dispatcher_->ResumeWorkCtxForRequest(request);

  auto& work_context = request->GetWorkCtx();
  work_context.coro_loop->PostTask(base::NewClosure([=]() {
    base::RefCoroutine coro = work_context.weak_coro.lock();
    if (coro) {
      base::CoroScheduler::TlsCurrent()->ResumeCoroutine(coro->Identifier());
    }
  }));
}

bool ClientRouter::SendClientRequest(RefProtocolMessage& message) {
  if (0 == channels_.size() || dispatcher_ == NULL) { //avoid x/0 Error
    LOG_IF(ERROR, !dispatcher_) << "No Dispatcher Can't Transfer This Request";
    LOG_IF(ERROR, 0 == channels_.size()) << " No ClientChannel Established"
                                         << " Server:" << server_addr_.IpPortAsString();
    return false;
  }

  uint32_t index = router_counter_ % channels_.size();

  RefClientChannel& client = channels_[index];
  base::MessageLoop2* io_loop = client->IOLoop();
  CHECK(io_loop);

  if (dispatcher_->PrepareOutRequestContext(message)) {
    LOG(FATAL) << "Can't Transfer/Dispatch This Request From Work To IO";
    return false;
  }

  router_counter_++;

  base::StlClourse func = std::bind(&ClientChannel::ScheduleARequest, client, message);

  dispatcher_->TransferAndYield(io_loop, func);

  //TODO: Check message status and wheather has a correct response for this request
  return true;
}

void ClientRouter::SetWorkLoadTransfer(CoroWlDispatcher* dispatcher) {
  dispatcher_ = dispatcher;
}

}

