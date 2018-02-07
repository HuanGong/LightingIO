#ifndef NET_COROWORKLOAD_DISPATCHER_H_H
#define NET_COROWORKLOAD_DISPATCHER_H_H

#include "workload_dispatcher.h"
#include "base/closure/closure_task.h"

namespace net {

class CoroWlDispatcher : public WorkLoadDispatcher {
public:
  CoroWlDispatcher(bool no_proxy);
  ~CoroWlDispatcher();

  bool Play(base::StlClourse& clourse) override;
  bool Dispatch(ProtoMessageHandler&, RefProtocolMessage&) override;

  //for client out-request and response-back
  bool PrepareOutRequestContext(RefProtocolMessage& message);
  void TransferAndYield(base::MessageLoop2* ioloop, base::StlClourse);

  bool ResumeWorkCtxForRequest(RefProtocolMessage& request);

  // brand new api
  void SetWorkContext(ProtocolMessage* message) override;
  bool TransmitToWorker(base::StlClourse& clourse) override;
private:
  void Reply(RefTcpChannel channel, RefProtocolMessage request);
  void DispachToCoroAndReply(ProtoMessageHandler, RefProtocolMessage);
};

}
#endif
