#ifndef _NET_IO_SERVICE_H_H
#define _NET_IO_SERVICE_H_H

#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include "net_callback.h"
#include "service_acceptor.h"
#include "dispatcher/workload_dispatcher.h"
#include "base/message_loop/message_loop.h"

namespace net {

class IOService;
/* IOserviceDelegate Provide the some notify to its owner and got the IOWork loop for IO*/
class IOServiceDelegate {
public:
  virtual ~IOServiceDelegate(){};
  virtual base::MessageLoop* GetNextIOWorkLoop() = 0;

  /* use for couting connection numbers and limit
   * max connections; return false when reach limits,
   * otherwise return true*/
  virtual bool IncreaseChannelCount() = 0;
  virtual void DecreaseChannelCount() = 0;

  virtual bool CanCreateNewChannel() { return true;}
  /* do other config things for this ioservice; return false will kill this service*/
  virtual bool BeforeIOServiceStart(IOService* ioservice) {return true;};
  virtual void IOServiceStarted(const IOService* ioservice) {};
  virtual void IOServiceStoped(const IOService* ioservice) {};
};

class IOService {
public:
  /* Must Construct in ownerloop, why? bz we want all io level is clear and tiny
   * it only handle io relative things, it's easy! just post a task IOMain at everything
   * begin */
  IOService(const InetAddress local,
            const std::string protocol,
            base::MessageLoop* workloop,
            IOServiceDelegate* delegate);

  ~IOService();

  void StartIOService();
  void StopIOService();

  base::MessageLoop* AcceptorLoop() { return acceptor_loop_; }
  const std::string& IOServiceName() const {return service_name_;}

  void SetWorkLoadDispatcher(WorkLoadDispatcher* d);
  void SetProtoMessageHandler(ProtoMessageHandler handler);
protected:
  void HandleRequest(const RefProtocolMessage& request);
  void HandleRequestOnWorker(const RefProtocolMessage request);
private:
  //void HandleProtoMessage(RefProtocolMessage message);
  /* create a new connection channel */
  void OnNewConnection(int, const InetAddress&);

  void OnChannelClosed(const RefTcpChannel&);

  /* store[remove] connection to[from] maps, only run on work_loop */
  void StoreConnection(const RefTcpChannel connetion);
  void RemoveConncetion(const RefTcpChannel connection);

  //bool as_dispatcher_;
  std::string protocol_;
  RefServiceAcceptor acceptor_;

  base::MessageLoop* acceptor_loop_;

  /* interface to owner and handler */
  IOServiceDelegate* delegate_;
  //RefProtoService proto_service_;

  WorkLoadDispatcher* dispatcher_;
  // install this callback to protoservice
  ProtoMessageHandler message_handler_;

  std::atomic<int64_t> channel_count_;
  std::string service_name_;
  bool is_stopping_;
  std::unordered_set<RefTcpChannel> connections_;
  //std::unordered_map<std::string, RefTcpChannel> connections_;
};

}
#endif
