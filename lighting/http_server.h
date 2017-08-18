#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include "ev_loop.h"
#include <memory>
#include <vector>
#include "event2/http.h"

#include "base/message_loop/message_loop.h"

namespace net {

struct SrvConfig {
  /*
  SrvConfig() :
    loop_per_port(false),
    hander_workers(1) {}
    */
  bool loop_per_port;
  std::vector<int> ports;
  uint32_t hander_workers;
};


template<class T> using UniquePtrList = std::vector<std::unique_ptr<T>>;

class HttpSrvDelegate {
  public:
    virtual ~HttpSrvDelegate() {}

    virtual void AfterIOLoopRun();
    virtual void BeforeIOLoopRun();
};

class HttpSrv {
public:
  HttpSrv(HttpSrvDelegate* delegate, SrvConfig& config_);
  ~HttpSrv();

  void Run();

  void InstallPath(const std::string& path);
private:
  struct EvHttpSrv {
    struct evhttp* ev_http_;
    std::unique_ptr<base::MessageLoop> io_loop_;
    struct evhttp_bound_socket* evhttp_bound_socket_;
  };

  void SetUpHttpSrv(EvHttpSrv* srv);
  static void GenericCallback(struct evhttp_request* req, void* arg);

  UniquePtrList<base::MessageLoop> workers_;

  SrvConfig config_;
  HttpSrvDelegate* delegate_;

  EvHttpSrv ev_http_server_;
};

}
#endif
