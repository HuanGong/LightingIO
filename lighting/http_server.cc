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

void Replyrequest(struct evhttp_request* req) {
  //HTTP header
  //evhttp_add_header(req->output_headers, "Server", "bad");
  evhttp_add_header(req->output_headers, "Connection", "keep-alive");
  evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
  //evhttp_add_header(req->output_headers, "Connection", "close");
  //输出的内容
  struct evbuffer *buf = evbuffer_new();
  evbuffer_add(buf, "hello work!", sizeof("hello work!"));
  evhttp_send_reply(req, HTTP_OK, "OK", buf);
  evbuffer_free(buf);
}

//typedef std::function <void(RequestContext*)> HTTPRequestHandler;
void Server::DistributeHttpReqeust(RequestContext* reqeust_ctx) {
  std::atomic<long> qps;
  LOG(INFO) << "Distribute a http reqeust";

  delete reqeust_ctx;
/*
  static long query_count = 0;
  query_count++;
#if 1
  HttpSrv* server = static_cast<HttpSrv*>(arg);

  //round-robin
  auto& worker = server->workers_[query_count%server->config_.hander_workers];

  auto f = [&](base::MessageLoop* ioloop, struct evhttp_request* req) {
    ioloop->PostTask(base::NewClosure(std::bind(Replyrequest, req)));
  };

  worker->PostTask(base::NewClosure(std::bind(f, base::MessageLoop::Current(), req)));
#else
  //HTTP header
  evhttp_add_header(req->output_headers, "Server", "bad");
  evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
  //输出的内容
  struct evbuffer *buf = evbuffer_new();
  evbuffer_add(buf, "hello work!", sizeof("hello work!"));
  evhttp_send_reply(req, HTTP_OK, "OK", buf);
  evbuffer_free(buf);
#endif
*/
}



}//endnamespace
