
#include "server.h"
#include "io_service.h"
#include "glog/logging.h"
#include "inet_address.h"
#include "url_string_utils.h"
#include "event_loop/linux_signal.h"

namespace net {

Server::Server(SrvDelegate* delegate)
  : quit_(false),
    delegate_(delegate) {

  CHECK(delegate_);
  comming_connections_.store(0);
  Initialize();
}

Server::~Server() {

}

void Server::Initialize() {

  InitIOWorkerLoop();

  InitIOServiceLoop();

  delegate_->RegisterProtoService(proto_services_);

  base::Signal::signal(SERVER_EXIT_SIG,
                       std::bind(&Server::ExitServerSignalHandler, this));
}

  /* formart scheme://ip:port http://0.0.0.0:5005 */
bool Server::RegisterService(const std::string server) {
  url::SchemeIpPort sch_ip_port;
  if (!url::ParseSchemeIpPortString(server, sch_ip_port)) {
    LOG(ERROR) << "argument format error,eg [scheme://xx.xx.xx.xx:port]";
    return false;
  }

  RefProtoService proto_servie = proto_services_[sch_ip_port.scheme];
  if (!proto_servie) {
    LOG(ERROR) << "No ProtoService Find, scheme:" << sch_ip_port.scheme;
    return false;
  }
  //delegate_->SetProtoMessageHandler(proto_servie);
  if (0 == ioservice_loops_.size()) {
    LOG(ERROR) << "No Loops Find For IOSerivce";
    return false;
  }
  RefMessageLoop loop = ioservice_loops_[ioservices_.size() % ioservice_loops_.size()];

  net::InetAddress listenner_addr(sch_ip_port.ip, sch_ip_port.port);

  RefIOService s(new IOService(listenner_addr, sch_ip_port.scheme, loop.get(), this));

  ioservices_.push_back(std::move(s));
  return true;
}

void Server::RunAllService() {
  for (auto& server : ioservices_) {
    server->StartIOService();
  }

  while(!quit_) {
    sleep(5);
  }
}

bool Server::IncreaseChannelCount() {
  comming_connections_++;
  LOG(INFO) << "Now Has <" << comming_connections_ << "> Comming Connections";
}

void Server::DecreaseChannelCount() {
  comming_connections_--;
  LOG(INFO) << "Now Has <" << comming_connections_ << "> Comming Connections";
}

base::MessageLoop2* Server::GetNextIOWorkLoop() {
  return ioworker_loops_[comming_connections_%ioworker_loops_.size()].get();
}

RefProtoService Server::GetProtocolService(const std::string protocol) {
  return proto_services_[protocol];
}

void Server::IOServiceStarted(const IOService* s) {
  LOG(INFO) << "IOService " << s->IOServiceName() << " Started .......";
};
void Server::IOServiceStoped(const IOService* s) {
  LOG(INFO) << "IOService " << s->IOServiceName() << " Stoped .......";
};


void Server::InitIOWorkerLoop() {
  const static std::string name_prefix("IOWorkerLoop:");
  int count = delegate_->GetIOWorkerLoopCount();
  for (int i = 0; i < count; i++) {
    RefMessageLoop loop(new base::MessageLoop2);
    loop->SetLoopName(name_prefix + std::to_string(i));
    loop->Start();
    ioworker_loops_.push_back(std::move(loop));
  }
  LOG(INFO) << "Server Create [" << ioworker_loops_.size() << "] loops Handle socket IO";
}

void Server::InitIOServiceLoop() {
  const static std::string name_prefix("IOServiceLoop:");
  int count = delegate_->GetIOServiceLoopCount();
  for (int i = 0; i < count; i++) {
    RefMessageLoop loop(new base::MessageLoop2);
    loop->SetLoopName(name_prefix + std::to_string(i));
    loop->Start();
    ioservice_loops_.push_back(std::move(loop));
  }
  LOG(INFO) << "Server Create [" << ioservice_loops_.size() << "] loops Accept new connection";
}

void Server::ExitServerSignalHandler() {
  for (auto& ioservice : ioservices_) {
    auto functor = std::bind(&IOService::StopIOService, ioservice);
    ioservice->AcceptorLoop()->PostTask(base::NewClosure(std::move(functor)));
  }
}

}//endnamespace net
