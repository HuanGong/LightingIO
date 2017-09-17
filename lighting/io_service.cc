#include "io_service.h"

#include "glog/logging.h"

namespace net {

//static run on io messageloop
void IoService::EvRequestHandler(evhttp_request* req, void* arg) {
  LOG(INFO) << "Get A REquest From Client";
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
    evbuffer_add(buf, "hello", sizeof("hello"));
    evhttp_send_reply(req, HTTP_NOCONTENT, "HTTP_NOCONTENT", buf);
    evbuffer_free(buf);
    return;
  }

  RequestContext* request_ctx = new RequestContext;
  request_ctx->ioservice = this;
  request_ctx->plat_req = req;
  request_ctx->from_io = loop_;
  //build url reqeust from libevent httpreqeust
  request_ctx->url_request = CreateFromNative(req);

  evhttp_add_header(req->output_headers, "Connection", "keep-alive");
  evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
  evhttp_add_header(req->output_headers, "Connection", "close");

  //输出的内容
  struct evbuffer *buf = evbuffer_new();
  //evbuffer_add(buf, "", sizeof("!"));
  evhttp_send_reply(req, HTTP_NOCONTENT, "HTTP_NOCONTENT", buf);
  evbuffer_free(buf);
  delete request_ctx;
  //request_callback_(request_ctx);
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
  std::size_t found = addr_port.find("::");
  if (found != std::string::npos) {
    addr_ = std::string(addr_port.begin(), addr_port.begin() + found);
    port_ = std::atoi(std::string(addr_port.begin() + found + 2, addr_port.end()).c_str());
  } else {
    LOG(INFO) << "parse address info failed, use 0.0.0.0::6666 as default";
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

  //int evhttp_parse_query(const char *uri, struct evkeyvalq *headers);

  //path
  const evhttp_uri *ev_uri = evhttp_request_get_evhttp_uri(req);
  const char* path = evhttp_uri_get_path(ev_uri);
  http_req->SetURLPath(path == NULL ? "\\" : path);

  //scheme
  const char *scheme = evhttp_uri_get_scheme(ev_uri);
  LOG(INFO) << "scheme is:" << scheme;

  //const char *evhttp_request_get_uri(const struct evhttp_request *req);
  //struct evhttp_uri *evhttp_request_get_evhttp_uri(const struct evhttp_request *req);

  // extrac headers
  KeyValMap& http_headers = http_req->MutableHeaders();
  struct evkeyvalq* ev_headers = evhttp_request_get_input_headers(req);
  for (evkeyval* kv = ev_headers->tqh_first; kv != NULL; kv = kv->next.tqe_next) {
    http_headers.insert(std::pair<std::string, std::string>(kv->key, kv->value));
  }

  //get body
  ev_ssize_t m_len = EVBUFFER_LENGTH(req->input_buffer);
  http_req->SetContentBody((char*)EVBUFFER_DATA(req->input_buffer), m_len);
  LOG(INFO) << "Body" << http_req->ContentBody();

  /*
  std::string http_body;
  http_body.reserve(m_len+1);
  http_body.assign((char*)EVBUFFER_DATA(req->input_buffer), m_len);
  http_body[m_len] = '\0';

  LOG(INFO) << "get body:" << http_body;
  http_req->SetContentBody(http_body, m_len);
  */

  const char* host = evhttp_request_get_host(req);
  if (host) {
    http_req->SetHost(host);
  }

  std::stringstream ss;
  http_req->Dump2Sstream(ss);
  LOG(INFO) << "http reqeust body:" << ss.str();

  return std::move(http_req);
}

} //end namespace net
