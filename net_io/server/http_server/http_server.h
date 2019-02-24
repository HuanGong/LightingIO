#ifndef NET_HTTP_SERVER_H_H
#define NET_HTTP_SERVER_H_H

#include <list>
#include <vector>
#include <chrono>             // std::chrono::seconds
#include <functional>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status

#include "io_service.h"
#include "base/base_micro.h"
#include "protocol/proto_message.h"
#include "protocol/http/http_request.h"
#include "protocol/http/http_response.h"
#include "base/message_loop/message_loop.h"

namespace net {

typedef std::function<void(const HttpRequest*, HttpResponse*)> HttpMessageHandler;

//?? use traits replace IOserviceDelegate override?
class HttpServer : public IOServiceDelegate {
public:
  HttpServer();
  ~HttpServer();
  void SetIoLoops(std::vector<base::MessageLoop*>& loops);
  void SetDispatcher(WorkLoadDispatcher* dispatcher);

  void ServeAddress(const std::string, HttpMessageHandler);
  void ServeAddressSync(const std::string, HttpMessageHandler);
  void StopServer();
  void StopServerSync();
protected:
  //override from ioservice delegate
  bool CanCreateNewChannel() override;
  bool IncreaseChannelCount() override;
  void DecreaseChannelCount() override;
  base::MessageLoop* GetNextIOWorkLoop() override;
  bool BeforeIOServiceStart(IOService* ioservice) override;
  void IOServiceStarted(const IOService* ioservice) override;
  void IOServiceStoped(const IOService* ioservice) override;
  void OnRequestMessage(const RefProtocolMessage& request) override;

  // handle http request in target loop
  void HandleHttpRequest(const RefProtocolMessage message);
private:
  WorkLoadDispatcher* dispatcher_;
  HttpMessageHandler message_handler_;

  std::mutex mtx_;
  std::condition_variable cv_;

  std::vector<base::MessageLoop*> io_loops_;
  std::list<RefIOService> ioservices_;
  std::atomic_bool serving_flag_;
  std::atomic_uint32_t connection_count_;
  DISALLOW_COPY_AND_ASSIGN(HttpServer);
};

}
#endif
