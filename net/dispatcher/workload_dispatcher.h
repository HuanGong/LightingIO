#ifndef NET_WORKLOAD_DISPATCHER_H
#define NET_WORKLOAD_DISPATCHER_H

#include <atomic>
#include <cinttypes>
#include "net_callback.h"
#include "protocol/proto_message.h"
#include "base/message_loop/message_loop.h"

namespace net {

class WorkLoadDispatcher {
public:
  WorkLoadDispatcher(bool work_in_io);
  virtual ~WorkLoadDispatcher() {};


  base::MessageLoop* GetNextWorkLoop();
  bool HandleWorkInIOLoop() const {return work_in_io_;}
  void SetWorkerLoops(std::vector<base::MessageLoop*>& loops) {work_loops_ = loops;};

  // must call at Worker Loop, may ioworker or woker according to HandleWorkInIOLoop
  virtual bool SetWorkContext(WorkContext& ctx);
  virtual bool SetWorkContext(ProtocolMessage* message);
  // transmit task from IO TO worker loop
  virtual bool TransmitToWorker(base::StlClosure& closuse);
private:
  const bool work_in_io_;
  std::vector<base::MessageLoop*> work_loops_;
  std::atomic<uint64_t> round_robin_counter_;
};

}//end namespace net
#endif
