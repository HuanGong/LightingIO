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

HttpServer::HttpServer(SrvDelegate* delegate)
  : Server("http"),
    delegate_(delegate) {
  if (nullptr == delegate_) {
    delegate_ = &default_delegate;
  }
}

HttpServer::~HttpServer() {
//clean up things
}

void HttpServer::Initialize() {
  CHECK(delegate_);
  // init ioservice
  auto listen_addrs = delegate_->HttpListenAddress();
  if (!listen_addrs.size()) {
    LOG(ERROR) << "Please Provide A listen_addrs in SrvDelegate::HttpListenAddress";
    return;
  }

  InitIOService(listen_addrs);

  int32_t count = delegate_->CreateWorkersForServer(workers_);
  LOG(INFO) << "HttpServer Has [" << count << "] worker threads";
}

void HttpServer::InitIOService(std::vector<std::string>& addr_ports) {
  CHECK(delegate_);
  CHECK(!addr_ports.empty());
  LOG(INFO) << "InitIOService addrs size:" << addr_ports.size();

  for (auto& addr_port : addr_ports) {
    LOG(INFO) << "InitIOService:" << addr_port;
    IoService* srv = new IoService(addr_port);

    auto dispatcher = std::bind(&HttpServer::DistributeHttpReqeust,
                                this,
                                std::placeholders::_1);
    srv->RegisterHandler(dispatcher);

    ioservices.push_back(srv);
  }
}

void HttpServer::Run() {

  for (auto& service : ioservices) {
    service->StartIOService();
    delegate_->OnIoServiceRun(service);
  }
}

void HttpServer::Stop() {

}

void HttpServer::CleanUp() {

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
void HttpServer::DistributeHttpReqeust(RequestContext* request_ctx) {
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
