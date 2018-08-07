
#include "server.h"
#include "io_service.h"
#include "glog/logging.h"
#include "inet_address.h"
#include "url_string_utils.h"
#include "message_loop/linux_signal.h"
#include "protocol/proto_service_factory.h"

namespace net {

Server::Server(SrvDelegate* delegate)
  : quit_(false),
    delegate_(delegate) {

  CHECK(delegate_);
  client_loop_index_.store(0);
  comming_connections_.store(0);

  Initialize();
}

Server::~Server() {
  LOG(INFO) << " Server Gone";
}

void Server::Initialize() {

  InitIOWorkerLoop();

  InitIOServiceLoop();

  base::Signal::signal(SERVER_EXIT_SIG,
                       std::bind(&Server::ExitServerSignalHandler, this));
}

base::MessageLoop* Server::NextIOLoopForClient() {
  if (0 == ioworker_loops_.size()) {
    return NULL;
  }
  uint32_t idx = client_loop_index_++;
  return ioworker_loops_[idx % ioworker_loops_.size()].get();
}

bool Server::RegisterService(const std::string server, ProtoMessageHandler handler) {
  url::SchemeIpPort sch_ip_port;
  if (!url::ParseSchemeIpPortString(server, sch_ip_port)) {
    LOG(ERROR) << "argument format error,eg [scheme://xx.xx.xx.xx:port]";
    return false;
  }

  if (!ProtoServiceFactory::Instance().HasProtoServiceCreator(sch_ip_port.scheme)) {
    LOG(ERROR) << "No ProtoServiceCreator Find for protocol scheme:" << sch_ip_port.scheme;
    return false;
  }

  if (0 == ioservice_loops_.size()) {
    LOG(ERROR) << "No Loops Find For IOSerivce";
    return false;
  }

  net::InetAddress listenner_addr(sch_ip_port.ip, sch_ip_port.port);

#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
  for (auto& ref_loop : ioservice_loops_) {

    RefIOService s(new IOService(listenner_addr,
                                 sch_ip_port.scheme,
                                 ref_loop.get(),
                                 this));

    s->SetProtoMessageHandler(handler);
    s->SetWorkLoadDispatcher(delegate_->WorkLoadTransfer());

    ioservices_.push_back(std::move(s));
  }
#else
  RefMessageLoop loop = ioservice_loops_[ioservices_.size() % ioservice_loops_.size()];

  RefIOService s(new IOService(listenner_addr, sch_ip_port.scheme, loop.get(), this));
  s->SetProtoMessageHandler(handler);
  s->SetWorkLoadDispatcher(delegate_->WorkLoadTransfer());

  ioservices_.push_back(std::move(s));
#endif

  return true;
}

void Server::RunAllService() {

  for (auto& service : ioservices_) {
    service->StartIOService();
  }
}

bool Server::IncreaseChannelCount() {
  comming_connections_++;
  LOG(INFO) << "New Connection Now Has <" << comming_connections_ << "> Comming Connections";
  return true;
}

void Server::DecreaseChannelCount() {
  comming_connections_--;
  LOG(INFO) << "A connection Gone, Now Has <" << comming_connections_ << "> Comming Connections";
}

base::MessageLoop* Server::GetNextIOWorkLoop() {

#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
  return base::MessageLoop::Current();
#else
  return ioworker_loops_[comming_connections_%ioworker_loops_.size()].get();
#endif
}

void Server::IOServiceStarted(const IOService* s) {
  LOG(INFO) << "IOService " << s->IOServiceName() << " Started .......";
};

//TODO: not thread safe
void Server::IOServiceStoped(const IOService* s) {
  LOG(INFO) << "IOService " << s->IOServiceName() << " Stoped .......";
  auto iter = ioservices_.begin();
  for (;iter != ioservices_.end(); iter++) {
    if (iter->get() == s) {
      ioservices_.erase(iter);
      break;
    }
  }
  if (0 == ioservices_.size()) {
    quit_ = true;
    delegate_->OnServerStoped();
  }
};


void Server::InitIOWorkerLoop() {
  const static std::string name_prefix("IOWorkerLoop:");
  int count = delegate_->GetIOWorkerLoopCount();
  for (int i = 0; i < count; i++) {
    RefMessageLoop loop(new base::MessageLoop);
    loop->SetLoopName(name_prefix + std::to_string(i));
    loop->Start();
    ioworker_loops_.push_back(std::move(loop));
  }
  LOG(INFO) << "Server Create [" << ioworker_loops_.size() << "] loops Handle socket IO";
}

void Server::InitIOServiceLoop() {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
  ioservice_loops_ = ioworker_loops_;
  LOG(INFO) << "Server Use IOWrokerLoop Handle New Connection...";
#else
  const static std::string name_prefix("IOServiceLoop:");
  int count = delegate_->GetIOServiceLoopCount();
  for (int i = 0; i < count; i++) {
    RefMessageLoop loop(new base::MessageLoop);
    loop->SetLoopName(name_prefix + std::to_string(i));
    loop->Start();
    ioservice_loops_.push_back(std::move(loop));
  }
  LOG(INFO) << "Server Create [" << ioservice_loops_.size() << "] loops Accept new connection";
#endif
}

void Server::ExitServerSignalHandler() {

  delegate_->ServerIsGoingExit();

  for (auto& ioservice : ioservices_) {
    auto functor = std::bind(&IOService::StopIOService, ioservice);
    ioservice->AcceptorLoop()->PostTask(base::NewClosure(std::move(functor)));
  }
}

const std::vector<RefMessageLoop> Server::IOworkerLoops() const {
  return ioworker_loops_;
}

}//endnamespace net
