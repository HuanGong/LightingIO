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
#include "clients/client_router.h"
#include "message_loop/message_loop.h"

namespace net {

typedef std::shared_ptr<base::MessageLoop> RefMessageLoop;
typedef std::map<std::string, RefProtoService> ProtoserviceMap;

#define SERVER_EXIT_SIG 44

class Server;
//class IOServiceDelegate;

class SrvDelegate {
public:
  virtual ~SrvDelegate() {};

  virtual WorkLoadDispatcher* WorkLoadTransfer() {
    if (dispatcher_) {
      return dispatcher_.get();
    }
    //dispatcher_.reset(new CoroWlDispatcher(HandleRequstInIO()));
    dispatcher_.reset(new WorkLoadDispatcher(HandleRequstInIO()));
    dispatcher_->InitWorkLoop(GetWorkerLoopCount());
    dispatcher_->StartDispatcher();
    return dispatcher_.get();
  };

  virtual bool HandleRequstInIO() {return true;}
  /* WorkLoop Handle request/response from IOWorkerLoop*/
  virtual int GetWorkerLoopCount() {return 2;}
  /* IOService Using for Accept New Comming Connections */
  virtual int GetIOServiceLoopCount() {return std::thread::hardware_concurrency();}
  //IOWorkLoop handle socket IO and the encode/decode to requestmessage or responsemessage
  virtual int GetIOWorkerLoopCount() {return std::thread::hardware_concurrency();};

  virtual void ServerIsGoingExit() {};
  virtual void OnServerStoped() {};

  virtual void BeforeServerRun() {};
  virtual void AfterServerRun() {};
private:
  std::unique_ptr<WorkLoadDispatcher> dispatcher_;
};

class Server : public IOServiceDelegate,
               public RouterDelegate {
public:
  Server(SrvDelegate* delegate);
  ~Server();

  /* formart scheme://ip:port eg: http://0.0.0.0:5005 */
  bool RegisterService(const std::string, ProtoMessageHandler);

  void RunAllService();

  const std::vector<RefMessageLoop> IOworkerLoops() const;
public: //override from ioservicedelegate
  bool IncreaseChannelCount() override;
  void DecreaseChannelCount() override;
  base::MessageLoop* GetNextIOWorkLoop() override;

  bool CanCreateNewChannel() { return true;}
  void IOServiceStarted(const IOService* ioservice) override;
  void IOServiceStoped(const IOService* ioservice) override;

  //override form client routerdelegate
  base::MessageLoop* NextIOLoopForClient() override;
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

  std::atomic<uint32_t> client_loop_index_;
  std::atomic<uint64_t> comming_connections_;
};


}
#endif
