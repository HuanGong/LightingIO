#ifndef NET_COROWORKLOAD_DISPATCHER_H_H
#define NET_COROWORKLOAD_DISPATCHER_H_H

#include "workload_dispatcher.h"
#include "base/closure/closure_task.h"

namespace net {

class CoroWlDispatcher : public WorkLoadDispatcher {
public:
  CoroWlDispatcher(bool handle_in_io);
  ~CoroWlDispatcher();

  void TransferAndYield(base::MessageLoop* ioloop, base::StlClourse);
  bool ResumeWorkCtxForRequest(RefProtocolMessage& request);

  // brand new api
  bool SetWorkContext(ProtocolMessage* message) override;
  bool TransmitToWorker(base::StlClourse& clourse) override;
};

}
#endif
