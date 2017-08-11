

#include "http_server.h"
#include "event2/http_struct.h"
#include "unistd.h"
#include "stdlib.h"
#include <memory>

namespace net {

HttpSrv::HttpSrv(HttpSrvDelegate* delegate, SrvConfig& config)
  : delegate_(delegate),
    config_(config) {

  for (int i= 0; i < config_.hander_workers; i++) {
    std::unique_ptr<IO::EventLoop> loop(new IO::EventLoop("workerthread"));
    loop->Start();
    workers_.push_back(std::move(loop));
  }
  ev_http_server_.io_loop_.reset(new IO::EventLoop("httpserver io loop"));
  ev_http_server_.io_loop_->Start();
  ev_http_server_.io_loop_->PostTask(
    IO::NewClosure(std::bind(&HttpSrv::SetUpHttpSrv, this, &ev_http_server_)));
}

HttpSrv::~HttpSrv() {

}

void HttpSrv::Run() {

}

void HttpSrv::InstallPath(const std::string& path) {

}

//run on io thread
void HttpSrv::SetUpHttpSrv(EvHttpSrv* server) {
  std::cout << "enter HttpSrv::SetUpHttpSrv" << std::endl;
  server->ev_http_ = evhttp_new(server->io_loop_->EventBase());
  if (!server->ev_http_) {
    return;
  }

  int listen_port = config_.ports[0];
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
  server->evhttp_bound_socket_ = evhttp_bind_socket_with_handle(server->ev_http_, "0.0.0.0", listen_port);
  if (!server->evhttp_bound_socket_) {
    return;
  }
#else
  if (evhttp_bind_socket(server->evhttp_, "0.0.0.0", listen_port) != 0) {
    return;
  }
#endif
  evhttp_set_gencb(server->ev_http_, &HttpSrv::GenericCallback, this);
  return;
}

void Replyrequest(struct evhttp_request* req) {
  std::cout << __FUNCTION__ << std::endl;
  //HTTP header
  evhttp_add_header(req->output_headers, "Server", "bad");
  evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
  evhttp_add_header(req->output_headers, "Connection", "close");
  //输出的内容
  struct evbuffer *buf = evbuffer_new();
  evbuffer_add(buf, "hello work!", sizeof("hello work!"));
  evhttp_send_reply(req, HTTP_OK, "OK", buf);
  evbuffer_free(buf);
}

//static
void HttpSrv::GenericCallback(struct evhttp_request* req, void* arg) {
  static long query_count = 0;
  query_count++;
/*
  HttpSrv* server = static_cast<HttpSrv*>(arg);

  std::cout << __FUNCTION__ << std::endl;
  //round-robin
  auto& worker = server->workers_[query_count%server->config_.hander_workers];

  auto f = [&](IO::EventLoop* ioloop, struct evhttp_request* req) {
    std::cout << __FUNCTION__ << std::endl;
    ioloop->PostTask(IO::NewClosure(std::bind(Replyrequest, req)));
  };


  worker->PostTask(IO::NewClosure(std::bind(f, IO::EventLoop::Current(), req)));
  */
  //HTTP header
  evhttp_add_header(req->output_headers, "Server", "bad");
  evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
  evhttp_add_header(req->output_headers, "Connection", "close");
  //输出的内容
  struct evbuffer *buf = evbuffer_new();
  evbuffer_add(buf, "hello work!", sizeof("hello work!"));
  evhttp_send_reply(req, HTTP_OK, "OK", buf);
  evbuffer_free(buf);
}



}//endnamespace
