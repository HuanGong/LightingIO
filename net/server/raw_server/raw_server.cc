#include "raw_server.h"
#include "io_service.h"
#include "glog/logging.h"
#include "inet_address.h"
#include "tcp_channel.h"
#include "url_string_utils.h"
#include "protocol/proto_service.h"
#include "message_loop/linux_signal.h"
#include "protocol/proto_service_factory.h"

namespace net {

RawServer::RawServer():
  dispatcher_(nullptr) {
  serving_flag_.store(false);
  connection_count_.store(0);
}

void RawServer::SetIoLoops(std::vector<base::MessageLoop*>& loops) {
  if (serving_flag_.load()) {
    LOG(ERROR) << "io loops can only set before server start";
    return;
  }
  io_loops_ = loops;
}

void RawServer::SetDispatcher(WorkLoadDispatcher* dispatcher) {
  dispatcher_ = dispatcher;
}

void RawServer::ServeAddressSync(const std::string addr, RawMessageHandler handler) {

  ServeAddress(addr, handler);

  std::unique_lock<std::mutex> lck(mtx_);
  while (cv_.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
    LOG(INFO) << "starting... ... ...";
  }
}

void RawServer::ServeAddress(const std::string address, RawMessageHandler handler) {

  bool served = serving_flag_.exchange(true);
  LOG_IF(ERROR, served) << " Server Can't Serve Twice";
  CHECK(!served);

  CHECK(handler);
  LOG_IF(ERROR, io_loops_.empty()) << "No loop handle socket io";
  CHECK(io_loops_.size() > 0);

  url::SchemeIpPort sch_ip_port;
  if (!url::ParseSchemeIpPortString(address, sch_ip_port)) {
    LOG(ERROR) << "address format error,eg [raw://xx.xx.xx.xx:port]";
    CHECK(false);
  }
  if (!ProtoServiceFactory::Instance().HasProtoServiceCreator(sch_ip_port.protocol)) {
    LOG(ERROR) << "No ProtoServiceCreator Find for protocol scheme:" << sch_ip_port.protocol;
    CHECK(false);
  }

  message_handler_ = handler;

  ProtoMessageHandler func = std::bind(&RawServer::OnRawRequest, this, std::placeholders::_1);

  net::InetAddress addr(sch_ip_port.host_ip, sch_ip_port.port);

  {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
    for (base::MessageLoop* loop : io_loops_) {
      RefIOService service = std::make_shared<IOService>(addr, "raw", loop, this);
      service->SetProtoMessageHandler(func);
      ioservices_.push_back(std::move(service));
    }
#else
    base::MessageLoop* loop = io_loops_[ioservices_.size() % io_loops_.size()];
    RefIOService service = std::make_shared<IOService>(addr, "raw", loop, this);
    service->SetProtoMessageHandler(func);
    ioservices_.push_back(std::move(service));
#endif

    for (RefIOService& service : ioservices_) {
      service->StartIOService();
    }
  }

}

void RawServer::OnRawRequest(const RefProtocolMessage& request) {
  VLOG(GLOG_VTRACE) << "IOService::HandleRequest handle a request";

  if (!dispatcher_) {
    HandleRawRequest(request);
    return;
  }

  base::StlClosure functor = std::bind(&RawServer::HandleRawRequest, this, request);
  if (false == dispatcher_->TransmitToWorker(functor)) {
    RefTcpChannel channel = request->GetIOCtx().channel.lock();
    if (channel) {
      channel->ShutdownChannel();
    }
    LOG(ERROR) << __FUNCTION__ << " dispatcher_->TransmitToWorker failed";
  }
}

void RawServer::HandleRawRequest(const RefProtocolMessage request) {

  RefTcpChannel channel = request->GetIOCtx().channel.lock();
  if (!channel || !channel->IsConnected()) {
    VLOG(GLOG_VERROR) << __FUNCTION__ << " Channel Has Broken After Handle Request Message";
    return;
  }

  RefProtocolMessage response = channel->GetProtoService()->DefaultResponse(request);
  do {
    if (dispatcher_ && !dispatcher_->SetWorkContext(request->GetWorkCtx())) {
      LOG(ERROR) << "Set WorkerContext failed";
      break;
    }
    message_handler_((RawMessage*)request.get(), (RawMessage*)response.get());
  } while(0);

  channel->GetProtoService()->BeforeReplyMessage(request.get(), response.get());
  bool close = channel->GetProtoService()->CloseAfterMessage(request.get(), response.get());

  if (channel->InIOLoop()) { //send reply directly

    bool send_success = channel->SendProtoMessage(response);
    if (close || !send_success) { //failed
      channel->ShutdownChannel();
    }

  } else  { //Send Response to Channel's IO Loop

    auto functor = [=]() {
      bool send_success = channel->SendProtoMessage(response);
      if (close || !send_success) { //failed
        channel->ShutdownChannel();
      }
    };
    channel->IOLoop()->PostTask(NewClosure(std::move(functor)));
  }
}

bool RawServer::CanCreateNewChannel() {
  return true;
}

base::MessageLoop* RawServer::GetNextIOWorkLoop() {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
  return base::MessageLoop::Current();
#else
  static std::atomic_uint32_t index = 0;
  return io_loops_[index.fetch_add(1) % io_loops_.size()];
#endif
}

bool RawServer::IncreaseChannelCount() {
  connection_count_.fetch_add(1);
  return true;
}

void RawServer::DecreaseChannelCount() {
  connection_count_.fetch_sub(1);
}

bool RawServer::BeforeIOServiceStart(IOService* ioservice) {
  return true;
}

void RawServer::IOServiceStarted(const IOService* service) {
  LOG(INFO) << "Raw Server IO Service " << service->IOServiceName() << " Started .......";

  { //sync
    std::unique_lock<std::mutex> lck(mtx_);
    for (auto& service : ioservices_) {
      if (!service->IsRunning()) return;
    }
    cv_.notify_all();
  }
}

void RawServer::IOServiceStoped(const IOService* service) {
  LOG(INFO) << "Raw Server IO Service " << service->IOServiceName() << " Stoped  .......";

  {
    std::unique_lock<std::mutex> lck(mtx_);
    ioservices_.remove_if([&](RefIOService& s) -> bool {
      return s.get() == service;
    });
  }

  { //sync
    std::unique_lock<std::mutex> lck(mtx_);
    for (auto& service : ioservices_) {
      if (service->IsRunning()) return;
    }
    cv_.notify_all();
  }
}

void RawServer::StopServerSync() {

  StopServer();

  std::unique_lock<std::mutex> lck(mtx_);
  while (cv_.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
    LOG(INFO) << "stoping... ... ...";
  }
}

void RawServer::StopServer() {
  for (auto& service : ioservices_) {
    service->StopIOService();
  }
}

} //end net
