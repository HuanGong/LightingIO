#include "http_server.h"
#include "event2/http_struct.h"
#include "unistd.h"
#include "stdlib.h"
#include <memory>
#include <string>
#include <functional>
#include "glog/logging.h"

namespace net {
static SrvDelegate default_delegate;
static std::string wokername_prefix("worker_");

Server::Server(SrvDelegate* delegate)
  : delegate_(delegate) {
  if (nullptr == delegate_) {
    delegate_ = &default_delegate;
  }
}

Server::~Server() {
//clean up things
}

void Server::InitWithAddrPorts(std::vector<std::string>& addr_ports) {
  CHECK(delegate_);
  CHECK(!addr_ports.empty());

  for (auto& addr_port : addr_ports) {

    IoService* srv = new IoService(addr_port);

    srv->RegisterHandler(std::bind(&Server::DistributeHttpReqeust, this, std::placeholders::_1));

    ioservices.push_back(srv);
  }
  InitializeWorkerService();
}

void Server::InitializeWorkerService() {
  CHECK(delegate_);
  int32_t worker_count = delegate_->WorkerCount();

  for (int idx = 0; idx < worker_count; idx++) {
    std::string worker_name = wokername_prefix + std::to_string(idx);
    std::unique_ptr<base::MessageLoop> loop(new base::MessageLoop(worker_name));
    workers_.push_back(std::move(loop));
  }
  //other settings for worker
}

void Server::Run() {

  for (auto& worker : workers_) {
    worker->Start();
    delegate_->OnWorkerStarted(worker.get());
  }

  for (auto& service : ioservices) {
    service->StartIOService();
    delegate_->OnIoServiceRun(service);
  }
}

void Server::Stop() {

}

void Server::CleanUp() {

}

void Replyrequest(RequestContext* req_ctx) {
  std::unique_ptr<HttpResponse> response(new HttpResponse(200));
  std::string body("helsdfadfadfa \n");
  response->SetResponseBody(body);

  auto& http_request = req_ctx->url_request;
  http_request->SetReplyResponse(std::move(response));

  req_ctx->ioservice->ReplyRequest(req_ctx);
}

//typedef std::function <void(RequestContext*)> HTTPRequestHandler;
void Server::DistributeHttpReqeust(RequestContext* request_ctx) {
  static std::atomic<uint32_t> qps;
  static std::atomic_int time_ms;
  if (!workers_.size()) {
    LOG(ERROR) << "no worker can handle this request";
    return;
  }

  int x = qps++ % workers_.size();
  LOG(ERROR) << x;

  base::MessageLoop* w = workers_[x].get();
  w->PostTask(base::NewClosure(std::bind(Replyrequest, request_ctx)));
}

}//endnamespace
