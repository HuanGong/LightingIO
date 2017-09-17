#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <map>
#include <memory>
#include <vector>
#include "event2/http.h"

#include "server.h"
#include "io_service.h"

#include "base/message_loop/message_loop.h"

namespace net {

class HttpServer : public Server {
public:
  HttpServer(SrvDelegate* delegate);
  ~HttpServer();

  //override from Class Server
  void Initialize() override;
  void Run() override;
  void Stop() override;

  void CleanUp();

protected:
  void InitIOService(std::vector<std::string>& addr_ports);
  void DistributeHttpReqeust(RequestContext* reqeust_ctx);

private:
  SrvDelegate* delegate_;
  bool service_per_loop_;

  WorkerContainer workers_;

  std::vector<IoService*> ioservices;
};

} //end net
#endif
