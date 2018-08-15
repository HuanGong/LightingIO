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
  typedef std::shared_ptr<base::MessageLoop> RefMessageLoop;
  WorkLoadDispatcher(bool work_in_io);
  virtual ~WorkLoadDispatcher() {};

  void InitWorkLoop(const int32_t count);
  void StartDispatcher();

  bool HandleWorkInIOLoop() const {return work_in_io_;}

  base::MessageLoop* GetNextWorkLoop();

  // must call at Worker Loop, may ioworker or woker according to HandleWorkInIOLoop
  virtual bool SetWorkContext(WorkContext& ctx);
  virtual bool SetWorkContext(ProtocolMessage* message);
  // transmit task from IO TO worker loop
  virtual bool TransmitToWorker(base::StlClosure& closuse);
private:
  bool work_in_io_;
  bool started_;
  std::vector<RefMessageLoop> work_loops_;
  std::atomic<uint64_t> round_robin_counter_;
};

}//end namespace net
#endif
