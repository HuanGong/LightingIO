#ifndef NET_RAW_SERVER_H_H
#define NET_RAW_SERVER_H_H

#include <list>
#include <mutex>              // std::mutex, std::unique_lock
#include <vector>
#include <chrono>             // std::chrono::seconds
#include <functional>

#include "base/base_micro.h"
#include "net_io/io_service.h"
#include "net_io/codec/codec_message.h"
#include "net_io/codec/raw/raw_message.h"
#include "base/message_loop/message_loop.h"

namespace lt {
namespace net {

class RawServerContext {
public:
  RawServerContext(const RefCodecMessage& req) : request_(req) {};

  bool IsResponded() const {return did_reply_;}

  template<typename T>
  const T* GetRequest() const {return (T*)request_.get();}

  void SendResponse(const RefCodecMessage& response);
private:
  void do_response(const RefCodecMessage& response);

  bool did_reply_ = false;
  RefCodecMessage request_;
};

typedef std::function<void(RawServerContext* context)> RawMessageHandler;
class RawServer : public IOServiceDelegate {
public:
  RawServer();

  void SetDispatcher(Dispatcher* dispatcher);
  void SetIOLoops(std::vector<base::MessageLoop*>& loops);
  void SetWorkLoops(std::vector<base::MessageLoop*>& loops);

  void ServeAddress(const std::string, RawMessageHandler);

  void StopServer();
  // set a callback if wan't got a callback when server stoped
  void SetCloseCallback(StlClosure callback);
protected:
  //override from ioservice delegate
  bool CanCreateNewChannel() override;
  void IncreaseChannelCount() override;
  void DecreaseChannelCount() override;
  base::MessageLoop* GetNextIOWorkLoop() override;
  void IOServiceStarted(const IOService* ioservice) override;
  void IOServiceStoped(const IOService* ioservice) override;
  void OnRequestMessage(const RefCodecMessage&) override;

  // handle raw request in target loop
  void HandleRawRequest(const RefCodecMessage message);
private:
  Dispatcher* dispatcher_;
  RawMessageHandler message_handler_;

  std::mutex mtx_;

  std::vector<base::MessageLoop*> io_loops_;
  std::list<RefIOService> ioservices_;
  StlClosure closed_callback_;

  std::atomic<bool> serving_flag_;
  std::atomic_uint32_t connection_count_;
  DISALLOW_COPY_AND_ASSIGN(RawServer);
};

}}
#endif
