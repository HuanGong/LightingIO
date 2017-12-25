#ifndef _NET_SERVER_SERVICE_H_H
#define _NET_SERVER_SERVICE_H_H

#include <map>
#include <thread>
#include <vector>
#include <atomic>
#include <inttypes.h>
#include "net_callback.h"
#include "io_service.h"
#include "event_loop/msg_event_loop.h"
#include "glog/logging.h"

namespace net {

typedef std::shared_ptr<base::MessageLoop2> RefMessageLoop;
typedef std::map<std::string, RefProtoService> ProtoserviceMap;

#define SERVER_EXIT_SIG 44

class Server;
//class IOServiceDelegate;

class SrvDelegate {
public:
  virtual ~SrvDelegate() {};

  /*add new or modify default protocol service*/
  virtual void RegisterProtoService(ProtoserviceMap& map) {
    LOG(INFO) << "base class RegisterProtoService be called";
  };
  virtual void SetProtoMessageHandler(RefProtoService& proto_service) {};

  virtual bool SharedLoopsForIO() {return false;};
  /* return n {n > 0} will create n loop using for acceptor,
   * return 0 or negtive value will using the IOWorker Loop Randomly*/
  virtual int GetIOServiceLoopCount() { return 1;}
  virtual int GetIOWorkerLoopCount() {return std::thread::hardware_concurrency();};

  virtual void BeforeServerRun() {};
  virtual void AfterServerRun() {};
};

class Server : public IOServiceDelegate {
public:
  Server(SrvDelegate* delegate);
  ~Server();

  void Initialize();
  /* formart scheme://ip:port http://0.0.0.0:5005 */
  bool RegisterService(const std::string server);

  void RunAllService();
public: //override from ioservicedelegate
  bool IncreaseChannelCount() override;
  void DecreaseChannelCount() override;
  base::MessageLoop2* GetNextIOWorkLoop() override;
  RefProtoService GetProtocolService(const std::string protocol) override;

  bool CanCreateNewChannel() { return true;}
  void IOServiceStarted(const IOService* ioservice) override;
  void IOServiceStoped(const IOService* ioservice) override;
private:
  void InitIOWorkerLoop();
  void InitIOServiceLoop();
  void ExitServerSignalHandler();

  bool quit_;
  SrvDelegate* delegate_;
  ProtoserviceMap proto_services_;
  std::vector<RefIOService> ioservices_;

  std::vector<RefMessageLoop> ioworker_loops_;
  std::vector<RefMessageLoop> ioservice_loops_;

  std::atomic<uint64_t> comming_connections_;
};


}
#endif
