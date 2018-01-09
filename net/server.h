#ifndef _NET_SERVER_SERVICE_H_H
#define _NET_SERVER_SERVICE_H_H

#include <map>
#include <thread>
#include <vector>
#include <atomic>
#include <inttypes.h>
#include "net_callback.h"
#include "io_service.h"
#include "dispatcher/coro_dispatcher.h"

#include "glog/logging.h"
#include "event_loop/msg_event_loop.h"

namespace net {

typedef std::shared_ptr<base::MessageLoop2> RefMessageLoop;
typedef std::map<std::string, RefProtoService> ProtoserviceMap;

#define SERVER_EXIT_SIG 44

class Server;
//class IOServiceDelegate;

class SrvDelegate {
public:
  virtual ~SrvDelegate() {};

  virtual bool HandleRequstInIO() {return false;}
  /* WorkLoop Handle request/response from IOWorkerLoop*/
  virtual int GetWorkerLoopCount() {return 4;/*std::thread::hardware_concurrency();*/}

  /* IOService Using for Handle New Comming Connections */
  virtual int GetIOServiceLoopCount() { return 4;}
  //IOWorkLoop handle socket IO and the encode/decode to requestmessage or responsemessage
  virtual int GetIOWorkerLoopCount() {return 4; /*std::thread::hardware_concurrency();*/};

  virtual void BeforeServerRun() {};
  virtual void AfterServerRun() {};
};

class Server : public IOServiceDelegate {
public:
  Server(SrvDelegate* delegate);
  ~Server();

  /* formart scheme://ip:port eg: http://0.0.0.0:5005 */
  bool RegisterService(const std::string, ProtoMessageHandler);

  void RunAllService();

public: //override from ioservicedelegate
  bool IncreaseChannelCount() override;
  void DecreaseChannelCount() override;
  base::MessageLoop2* GetNextIOWorkLoop() override;

  WorkLoadDispatcher* MessageDispatcher() override {
    return dispatcher_.get();
  };

  bool CanCreateNewChannel() { return true;}
  void IOServiceStarted(const IOService* ioservice) override;
  void IOServiceStoped(const IOService* ioservice) override;
private:
  void Initialize();

  void InitIOWorkerLoop();
  void InitIOServiceLoop();
  void ExitServerSignalHandler();

  bool quit_;
  SrvDelegate* delegate_;
  std::vector<RefIOService> ioservices_;

  std::vector<RefMessageLoop> ioworker_loops_;
  std::vector<RefMessageLoop> ioservice_loops_;

  std::atomic<uint64_t> comming_connections_;

  std::shared_ptr<CoroWlDispatcher> dispatcher_;
};


}
#endif
