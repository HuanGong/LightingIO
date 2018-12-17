#include "client_router.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_runner.h"

namespace net {

ClientRouter::ClientRouter(base::MessageLoop* loop, const InetAddress& server)
  : protocol_("http"),
    channel_count_(1),
    reconnect_interval_(5000),
    connection_timeout_(10000),
    message_timeout_(30000),
    server_addr_(server),
    work_loop_(loop),
    is_stopping_(false),
    delegate_(NULL),
    dispatcher_(NULL) {

  router_counter_ = 0;
  CHECK(work_loop_);
  connector_ = std::make_shared<Connector>(work_loop_, this);

  std::shared_ptr<ClientChannelList> list(new ClientChannelList(channels_));
  std::atomic_store(&roundrobin_channes_, list);
}

ClientRouter::~ClientRouter() {
}

void ClientRouter::SetDelegate(RouterDelegate* delegate) {
  delegate_ = delegate;
}

void ClientRouter::SetupRouter(const RouterConf& config) {
  protocol_ = config.protocol;
  channel_count_ = config.connections;
  reconnect_interval_ = config.recon_interval;
  message_timeout_ = config.message_timeout;
}

void ClientRouter::StartRouter() {
  for (uint32_t i = 0; i < channel_count_; i++) {
    auto functor = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
    work_loop_->PostTask(NewClosure(functor));
  }
}

void ClientRouter::StopRouter() {
  auto channels = std::atomic_load(&roundrobin_channes_);
  if (is_stopping_) {
    return;
  }
  is_stopping_ = true;
  for (auto& client : *channels) {
    client->CloseClientChannel();
  }
}

void ClientRouter::StopRouterSync() {

  StopRouter();

  std::unique_lock<std::mutex> lck(mtx_);
  cv_.wait(lck, [&]{
    auto channels = std::atomic_load(&roundrobin_channes_);
    return channels->empty();
  });
}

void ClientRouter::OnClientConnectFailed() {
  if (is_stopping_ || channel_count_ <= channels_.size()) {
    return;
  }
  VLOG(GLOG_VERROR) << __FUNCTION__ << RouterInfo() << " try after " << reconnect_interval_ << " ms";
  auto functor = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
  work_loop_->PostDelayTask(NewClosure(functor), reconnect_interval_);
}

base::MessageLoop* ClientRouter::GetLoopForClient() {
  base::MessageLoop* io_loop = NULL;
  if (delegate_) {
    io_loop = delegate_->NextIOLoopForClient();
  }
  return io_loop ? io_loop : work_loop_;
}

void ClientRouter::OnNewClientConnected(int socket_fd, InetAddress& local, InetAddress& remote) {
  CHECK(work_loop_->IsInLoopThread());

  base::MessageLoop* io_loop = GetLoopForClient();

  auto new_channel = TcpChannel::Create(socket_fd, local, remote, io_loop);

  RefProtoService proto_service = ProtoServiceFactory::Create(protocol_);
  proto_service->SetServiceType(ProtocolServiceType::kClient);
  proto_service->BindChannel(new_channel);

  RefClientChannel client_channel = CreateClientChannel(this, proto_service);
  client_channel->SetRequestTimeout(message_timeout_);

  proto_service->SetDelegate(client_channel.get());
  proto_service->StartHeartBeat(5000);

  client_channel->StartClient();

  channels_.push_back(client_channel);

  std::shared_ptr<ClientChannelList> list(new ClientChannelList(channels_));
  std::atomic_store(&roundrobin_channes_, list);

  VLOG(GLOG_VINFO) << __FUNCTION__ << RouterInfo() << " new protocol service started";
}

/*on the loop of client IO, need managed by connector loop*/
void ClientRouter::OnClientChannelClosed(const RefClientChannel& channel) {

  if (!work_loop_->IsInLoopThread()) {
    auto functor = std::bind(&ClientRouter::OnClientChannelClosed, this, channel);
    work_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }

  auto iter = std::find(channels_.begin(), channels_.end(), channel);
  if (iter != channels_.end()) {
    channels_.erase(iter);
  }

  std::shared_ptr<ClientChannelList> list(new ClientChannelList(channels_));
  std::atomic_store(&roundrobin_channes_, list);

  if (!is_stopping_ && channels_.size() < channel_count_) {
    VLOG(GLOG_VTRACE) << __FUNCTION__ << RouterInfo() << " reconnect after:" << reconnect_interval_;
    //auto functor = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
    //work_loop_->PostDelayTask(NewClosure(functor), reconnect_interval_);

    auto functor2 = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
    work_loop_->PostDelayTask(NewClosure(functor2), reconnect_interval_);
  }

  VLOG(GLOG_VINFO) << __FUNCTION__ << RouterInfo();

  if (is_stopping_ && channels_.empty()) {//for sync stoping
    cv_.notify_all();
  }
}

void ClientRouter::OnRequestGetResponse(const RefProtocolMessage& request,
                                        const RefProtocolMessage& response) {
  CHECK(request);
  request->SetResponse(response);

  dispatcher_->ResumeWorkContext(request->GetWorkCtx());
}

ProtocolMessage* ClientRouter::SendClientRequest(RefProtocolMessage& message) {
  CHECK(dispatcher_);

  if (!dispatcher_->SetWorkContext(message->GetWorkCtx())) {
    LOG(FATAL) << __FUNCTION__ << " this task can't by yield, send failed";
    return NULL;
  }

  auto channels = std::atomic_load(&roundrobin_channes_);

  if (channels->empty()) { //avoid x/0 Error
    LOG(ERROR) << " No Connection Established To Server:" << server_addr_.IpPortAsString();
    return NULL;
  }

  uint32_t count = router_counter_.fetch_add(1);
  RefClientChannel& client = (*channels)[count % (*channels).size()];
  base::MessageLoop* io_loop = client->IOLoop();
  CHECK(io_loop);

  base::StlClosure func = std::bind(&ClientChannel::SendRequest, client, message);
  dispatcher_->TransferAndYield(io_loop, func);

  return message->RawResponse();
}

void ClientRouter::SetWorkLoadTransfer(CoroWlDispatcher* dispatcher) {
  dispatcher_ = dispatcher;
}

uint32_t ClientRouter::ClientCount() const {
  auto channels = std::atomic_load(&roundrobin_channes_);
  return channels->size();
}

std::string ClientRouter::RouterInfo() const {
  std::ostringstream oss;
  oss << " [remote:" << server_addr_.IpPortAsString()
      << ", clients:" << ClientCount() << "]";
  return oss.str();
}

}//end namespace net
