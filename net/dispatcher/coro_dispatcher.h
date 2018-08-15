#ifndef NET_COROWORKLOAD_DISPATCHER_H_H
#define NET_COROWORKLOAD_DISPATCHER_H_H

#include "workload_dispatcher.h"
#include "base/closure/closure_task.h"

namespace net {

class CoroWlDispatcher : public WorkLoadDispatcher {
public:
  CoroWlDispatcher(bool handle_in_io);
  ~CoroWlDispatcher();

  void TransferAndYield(base::MessageLoop* ioloop, base::StlClosure);

  // brand new api
  bool SetWorkContext(WorkContext& ctx) override;
  bool SetWorkContext(ProtocolMessage* message) override;
  bool TransmitToWorker(base::StlClosure& clourse) override;
  bool ResumeWorkContext(WorkContext& ctx);
};

}
#endif
