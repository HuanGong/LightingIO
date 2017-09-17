#ifndef IO_SERVICE_H_H
#define IO_SERVICE_H_H

#include <memory>
#include <functional>
#include "request/url_request.h"
#include "base/message_loop/message_loop.h"


#include "event2/http.h"
#include "event2/http_struct.h"

namespace net {

class IoService;

typedef evhttp_request PlatformReqeust;

struct RequestContext {
  RequestContext()
    : ioservice(nullptr),
      plat_req(nullptr) {
  }
  IoService* ioservice;
  PlatformReqeust* plat_req;
  std::shared_ptr<base::MessageLoop> from_io;
  std::shared_ptr<HttpUrlRequest> url_request;
};

typedef std::function <void(RequestContext*)> HTTPRequestHandler;
typedef void (*EvHttpCallBack)(struct evhttp_request* req, void* arg);

class IoService {
public:
  IoService(std::string addr_port);
  ~IoService();

  static void EvRequestHandler(evhttp_request* req, void* arg);

  const std::string AddressPort();
  base::MessageLoop* IOServiceLoop();

  void StopIOservice();
  void StartIOService();

  void RegisterHandler(HTTPRequestHandler handler);
protected:
  void InitEvHttpServer();

  void ResolveAddressPorts(const std::string& addr_port);

  void HandleEvHttpReqeust(PlatformReqeust* req);
  std::shared_ptr<HttpUrlRequest> CreateFromNative(PlatformReqeust* req);

private:
  //server info
  uint32_t port_;
  std::string addr_;

  HTTPRequestHandler request_callback_;
  std::shared_ptr<base::MessageLoop> loop_;

  //platform relative
  struct evhttp* ev_http_;
  struct evhttp_bound_socket* bound_socket_;
};

}// end net
#endif
