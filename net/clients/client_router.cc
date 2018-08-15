#include "client_router.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_scheduler.h"

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
  message_timeout_ = config.message_timeout;
}

void ClientRouter::StartRouter() {
  for (uint32_t i = 0; i < channel_count_; i++) {
    auto functor = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
    work_loop_->PostTask(base::NewClosure(functor));
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

void ClientRouter::StopRouterSync() {

  StopRouter();

  std::unique_lock<std::mutex> lck(mtx_);
  while (cv_.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
    LOG(INFO) << "stoping... ... ...";
  }
}

void ClientRouter::OnClientConnectFailed() {
  if (is_stopping_ || channel_count_ <= channels_.size()) {
    return;
  }
  LOG(INFO) << server_addr_.IpPortAsString() << "A Connection Broken,Connect After " << reconnect_interval_ << " ms";
  auto functor = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
  work_loop_->PostDelayTask(base::NewClosure(functor), reconnect_interval_);
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

  RefTcpChannel new_channel = TcpChannel::CreateClientChannel(socket_fd,
                                                              local,
                                                              remote,
                                                              io_loop);
  new_channel->SetOwnerLoop(work_loop_);
  new_channel->SetChannelName(remote.IpPortAsString());

  RefClientChannel client_channel = ClientChannel::Create(this, new_channel);
  client_channel->SetRequestTimeout(message_timeout_);

  RefProtoService proto_service = ProtoServiceFactory::Create(protocol_);
  proto_service->SetServiceType(ProtocolServiceType::kClient);
  proto_service->SetMessageHandler(std::bind(&ClientChannel::OnResponseMessage,
                                             client_channel.get(),
                                             std::placeholders::_1));
  new_channel->SetProtoService(proto_service);

  new_channel->Start();

  VLOG(GLOG_VTRACE) << " ClientRouter::OnNewClientConnected, Channel:" << new_channel->ChannelName();
  channels_.push_back(client_channel);
  VLOG(GLOG_VINFO) << server_addr_.IpPortAsString() << " Has " << channels_.size() << " Channel";
}

void ClientRouter::OnClientChannelClosed(RefClientChannel channel) {
  if (!work_loop_->IsInLoopThread()) {
    auto functor = std::bind(&ClientRouter::OnClientChannelClosed, this, channel);
    work_loop_->PostTask(base::NewClosure(std::move(functor)));
    return;
  }

  auto iter = std::find(channels_.begin(), channels_.end(), channel);
  if (iter != channels_.end()) {
    channels_.erase(iter);
  }

  if (is_stopping_ && channels_.empty()) {//for sync stoping
    cv_.notify_all();
  }

  VLOG(GLOG_VINFO) << server_addr_.IpPortAsString() << " Has " << channels_.size() << " Channel";

  if (!is_stopping_ && channels_.size() < channel_count_) {
    VLOG(GLOG_VTRACE) << "Broken, ReConnect After:" << reconnect_interval_ << " ms";
    auto functor = std::bind(&Connector::LaunchAConnection, connector_, server_addr_);
    work_loop_->PostDelayTask(base::NewClosure(functor), reconnect_interval_);
  }
}

void ClientRouter::OnRequestGetResponse(RefProtocolMessage request,
                                        RefProtocolMessage response) {
  CHECK(request);

  request->SetResponse(response);
  dispatcher_->ResumeWorkContext(request->GetWorkCtx());
}

bool ClientRouter::SendClientRequest(RefProtocolMessage& message) {

  CHECK(dispatcher_);
  if (!dispatcher_->SetWorkContext(message->GetWorkCtx())) {
    LOG(FATAL) << "this task can't by yield, send failed";
    return false;
  }

  //schedule a timeout
  //work_loop_->PostTask(base::NewClosure(std::bind(&ClientRouter::SendRequestInWorkLoop, message)));
  base::StlClosure func = std::bind(&ClientRouter::SendRequestInWorkLoop, this, message);
  work_loop_->PostTask(base::NewClosure([]() {
    LOG(ERROR) << __FUNCTION__ << " client router work loop alive";
  }));
  dispatcher_->TransferAndYield(work_loop_, func);
  return true;
}

void ClientRouter::SendRequestInWorkLoop(RefProtocolMessage message) {
  CHECK(work_loop_->IsInLoopThread());

  LOG(ERROR) << __FUNCTION__ << " To Server:" << server_addr_.IpPortAsString();

  if (channels_.empty()) { //avoid x/0 Error
    LOG(ERROR) << " No Connection Established To Server:" << server_addr_.IpPortAsString();
    return;
  }

  uint32_t count = router_counter_.fetch_add(1);
  RefClientChannel& client = channels_[count % channels_.size()];
  base::MessageLoop* io_loop = client->IOLoop();
  CHECK(io_loop);
  base::StlClosure func = std::bind(&ClientChannel::ScheduleARequest, client, message);
  io_loop->PostTask(base::NewClosure(func));
}

void ClientRouter::SetWorkLoadTransfer(CoroWlDispatcher* dispatcher) {
  dispatcher_ = dispatcher;
}

}//end namespace net

