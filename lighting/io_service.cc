#include "io_service.h"

#include "glog/logging.h"
#include "base/closure/closure_task.h"

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
    evhttp_add_header(req->output_headers, "Connection", "close");
    evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");

    struct evbuffer *buf = evbuffer_new();
    evhttp_send_reply(req, HTTP_NOCONTENT, "HTTP_NOCONTENT", buf);
    evbuffer_free(buf);
    return;
  }

  RequestContext* request_ctx = new RequestContext;
  request_ctx->ioservice = this;
  request_ctx->plat_req = req;
  request_ctx->from_io = loop_;
  request_ctx->url_request = CreateFromNative(req);

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
    LOG(INFO) << "Create Server And Bind " << addr_ << ":" << port_ << " failed";
    return;
  }
#else
  if (evhttp_bind_socket(ev_http_, addr_.c_str(), port_) != 0) {
    LOG(INFO) << "Create Server And Bind " << addr_ << ":" << port_ << " failed";
    return;
  }
#endif
  evhttp_set_gencb(ev_http_, &IoService::EvRequestHandler, this);
  LOG(INFO) << "Listen On:" << addr_ << " Port:" << port_;
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
  std::size_t found = addr_port.find(":");
  if (found != std::string::npos) {
    addr_ = std::string(addr_port.begin(), addr_port.begin() + found);
    port_ = std::atoi(std::string(addr_port.begin() + found + 1, addr_port.end()).c_str());
  } else {
    LOG(INFO) << "parse address info failed, use 0.0.0.0:6666 as default";
    port_ = 6666;
    addr_ = "0.0.0.0";
  }
}

std::shared_ptr<HttpUrlRequest> IoService::CreateFromNative(PlatformReqeust* req) {
  std::shared_ptr<HttpUrlRequest> http_req(new HttpUrlRequest(RequestType::INCOMING_REQ));
  //method
  enum evhttp_cmd_type method = evhttp_request_get_command(req);
  http_req->SetRequestMethod((HttpMethod)method);

  //request url eg:
  const char* uri = evhttp_request_get_uri(req);
  http_req->SetRequestURL(uri);

  const evhttp_uri *ev_uri = evhttp_request_get_evhttp_uri(req);

  //params
  const char* params_str = evhttp_uri_get_query(ev_uri);
  evkeyvalq ev_params;
  int ret = evhttp_parse_query_str(params_str, &ev_params);
  if (0 == ret) {
    KeyValMap& params = http_req->MutableParams();
    for (evkeyval* kv = ev_params.tqh_first; kv != NULL; kv = kv->next.tqe_next) {
      params.insert(std::pair<std::string, std::string>(kv->key, kv->value));
    }
  } else {
    LOG(ERROR) << "parse params from url failed, raw url:" << std::string(uri);
  }

  //path
  const char* path = evhttp_uri_get_path(ev_uri);
  http_req->SetURLPath(path == NULL ? "\\" : path);

  //scheme
  const char* scheme = evhttp_uri_get_scheme(ev_uri);
  if (scheme) {
    LOG(INFO) << "scheme: " <<  std::string(scheme);
  }

  //headers
  KeyValMap& http_headers = http_req->MutableHeaders();
  struct evkeyvalq* ev_headers = evhttp_request_get_input_headers(req);
  for (evkeyval* kv = ev_headers->tqh_first; kv != NULL; kv = kv->next.tqe_next) {
    http_headers.insert(std::pair<std::string, std::string>(kv->key, kv->value));
  }

  //get body
  ev_ssize_t m_len = EVBUFFER_LENGTH(req->input_buffer);
  http_req->SetContentBody((char*)EVBUFFER_DATA(req->input_buffer), m_len);

  //host
  const char* host = evhttp_request_get_host(req);
  if (host) {
    http_req->SetHost(host);
  }

  std::stringstream ss;
  http_req->Dump2Sstream(ss);
  LOG(INFO) << "http reqeust is:\n" << ss.str();

  return std::move(http_req);
}

void IoService::ReplyRequest(RequestContext* req_ctx) {
  CHECK(req_ctx);
  auto& loop = req_ctx->from_io;
  if (!loop->IsInLoopThread()) {
    loop->PostTask(base::NewClosure(
        std::bind(&IoService::ReplyRequestInternal, this, req_ctx)));
  } else {
    ReplyRequestInternal(req_ctx);
  }
}

void IoService::ReplyRequestInternal(RequestContext* req_ctx) {
  PlatformReqeust* ev_request = req_ctx->plat_req;
  const HttpResponse& response = req_ctx->url_request->Response();

  auto& outgoing_headers = response.Headers();
  for (const auto& header_pair : outgoing_headers) {
    evhttp_add_header(ev_request->output_headers,
                      header_pair.first.c_str(),
                      header_pair.second.c_str());
  }
  //evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");

  struct evbuffer *buf = evbuffer_new();
  const auto& body = response.ResponseBody();
  if (body.size()) {
    evbuffer_add(buf, body.c_str(), body.size());
  }
  int code = response.Code();

  evhttp_send_reply(ev_request, code, "HTTP_NOCONTENT", buf);
  evbuffer_free(buf);
  delete req_ctx;
}


} //end namespace net
