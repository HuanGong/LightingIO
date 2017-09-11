#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <map>
#include <memory>
#include <vector>
#include "event2/http.h"

#include "io_service.h"

#include "base/message_loop/message_loop.h"

namespace net {

template<class T> using UniquePtrList = std::vector<std::unique_ptr<T>>;

class SrvDelegate {
public:
  virtual ~SrvDelegate() {}

  virtual uint32_t WorkerCount() {
    return std::thread::hardware_concurrency();
  }
  virtual void OnIoServiceInit(IoService* io_service) {};
  virtual void OnIoServiceRun(IoService* io_service) {};

  virtual void OnWorkerStarted(base::MessageLoop* worker) {};
  virtual void ServerGone() {};
};

class Server {
public:
  Server(SrvDelegate* delegate);
  ~Server();

  void InitWithAddrPorts(std::vector<std::string>& addr_ports);

  void Run();
  void Stop();
  void CleanUp();
private:
  void InitializeWorkerService();

  SrvDelegate* delegate_;
  bool service_per_loop_;

  //typedef std::function <void(RequestContext*)> HTTPRequestHandler;
  void DistributeHttpReqeust(RequestContext* reqeust_ctx);

  UniquePtrList<base::MessageLoop> workers_;

  std::vector<IoService*> ioservices;
};

}
#endif
