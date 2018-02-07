#ifndef NET_WORKLOAD_DISPATCHER_H
#define NET_WORKLOAD_DISPATCHER_H

#include <atomic>
#include <cinttypes>
#include "net_callback.h"
#include "base/event_loop/msg_event_loop.h"

namespace net {

class WorkLoadDispatcher {
public:
  typedef std::shared_ptr<base::MessageLoop2> RefMessageLoop;

  WorkLoadDispatcher(bool work_in_io);
  virtual ~WorkLoadDispatcher() {};

  void InitWorkLoop(const int32_t count);

  void StartDispatcher();
  bool HandleWorkInIOLoop() {return work_in_io_;}


  base::MessageLoop2* GetNextWorkLoop();
  virtual bool Dispatch(ProtoMessageHandler&, RefProtocolMessage&) = 0;

  virtual bool Play(base::StlClourse& clourse) = 0;


  // brand new dipatcher api
  // must call at Worker Loop, may ioworker or woker according to HandleWorkInIOLoop
  virtual void SetWorkContext(ProtocolMessage* message);
  // transmit task from IO TO worker loop
  virtual bool TransmitToWorker(base::StlClourse& clourse);

private:
  bool work_in_io_;
  bool started_;
  std::vector<RefMessageLoop> work_loops_;
  std::atomic<uint64_t> round_robin_counter_;
};

}//end namespace net
#endif
