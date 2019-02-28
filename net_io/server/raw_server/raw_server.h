#ifndef NET_RAW_SERVER_H_H
#define NET_RAW_SERVER_H_H

#include <list>
#include <vector>
#include <chrono>             // std::chrono::seconds
#include <functional>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status

#include "io_service.h"
#include "base/base_micro.h"
#include "protocol/proto_message.h"
#include "protocol/raw/raw_message.h"
#include "base/message_loop/message_loop.h"

namespace net {

typedef std::function<void(const LtRawMessage*, LtRawMessage*)> RawMessageHandler;

class RawServer : public IOServiceDelegate {
public:
  RawServer();

  void SetDispatcher(Dispatcher* dispatcher);
  void SetIOLoops(std::vector<base::MessageLoop*>& loops);
  void SetWorkLoops(std::vector<base::MessageLoop*>& loops);

  void ServeAddress(const std::string, RawMessageHandler);
  void ServeAddressSync(const std::string, RawMessageHandler);
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
  void OnRequestMessage(const RefProtocolMessage&) override;

  // handle raw request in target loop
  void HandleRawRequest(const RefProtocolMessage message);
private:
  Dispatcher* dispatcher_;
  RawMessageHandler message_handler_;

  std::mutex mtx_;
  std::condition_variable cv_;

  std::vector<base::MessageLoop*> io_loops_;
  std::list<IOServicePtr> ioservices_;

  std::atomic<bool> serving_flag_;
  std::atomic_uint32_t connection_count_;
  DISALLOW_COPY_AND_ASSIGN(RawServer);
};

}
#endif
