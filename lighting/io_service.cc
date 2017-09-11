#include "io_service.h"

#include "glog/logging.h"

namespace net {

//static run on io messageloop
void IoService::EvRequestHandler(evhttp_request* req, void* arg) {
  DCHECK(arg);
  IoService* service = static_cast<IoService*>(arg);
  service->HandleEvHttpReqeust(req);
}

// run on io message loop
void IoService::HandleEvHttpReqeust(evhttp_request* req) {

  if (!request_callback_) {
    evhttp_add_header(req->output_headers, "Connection", "keep-alive");
    evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
    evhttp_add_header(req->output_headers, "Connection", "close");

    //输出的内容
    struct evbuffer *buf = evbuffer_new();
    //evbuffer_add(buf, "", sizeof("!"));
    evhttp_send_reply(req, HTTP_NOCONTENT, "HTTP_NOCONTENT", buf);
    evbuffer_free(buf);
    return;
  }

  RequestContext* request_ctx = new RequestContext;
  request_ctx->ioservice = this;
  request_ctx->plat_req = req;
  request_ctx->from_io = loop_;

  //build url reqeust from libevent httpreqeust
  request_ctx->url_request = NULL;

  request_callback_(request_ctx);
}

IoService::IoService(std::string addr_port) {
  ResolveAddressPorts(addr_port);

  loop_.reset(new base::MessageLoop(addr_port));

  InitEvHttpServer();
}

void IoService::InitEvHttpServer() {
  if (nullptr == loop_.get()) {
    return;
  }

  ev_http_ = evhttp_new(loop_->EventBase());
  if (!ev_http_) {
    return;
  }
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
  bound_socket_ = evhttp_bind_socket_with_handle(ev_http_, addr_.c_str(), port_);
  if (!bound_socket_) {
    return;
  }
#else
  if (evhttp_bind_socket(ev_http_, addr_.c_str(), port_) != 0) {
    return;
  }
#endif
  evhttp_set_gencb(ev_http_, &IoService::EvRequestHandler, this);
}

IoService::~IoService() {

}

void IoService::RegisterHandler(HTTPRequestHandler handler) {
  request_callback_ = handler;
}

const std::string IoService::AddressPort() {
  return addr_ + "::" + std::to_string(port_);
}

base::MessageLoop* IoService::IOServiceLoop() {
  return loop_.get();
}

void IoService::StopIOservice() {

}

void IoService::StartIOService() {
  loop_->Start();
}

void IoService::ResolveAddressPorts(const std::string& addr_port) {
  std::size_t found = addr_port.rfind("::");
  if (found != std::string::npos) {
    addr_ = std::string(addr_port.begin(), addr_port.begin() + found);
    port_ = std::atoi(std::string(addr_port.begin() + found + 2, addr_port.end()).c_str());
  } else {
    LOG(INFO) << "parse address info failed, use 0.0.0.0::6666 as default";
    port_ = 6666;
    addr_ = "0.0.0.0";
  }
}

} //end namespace net
