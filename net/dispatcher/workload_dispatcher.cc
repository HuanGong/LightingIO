
#include "glog/logging.h"
#include "workload_dispatcher.h"

#include "net/protocol/proto_message.h"

namespace net {

WorkLoadDispatcher::WorkLoadDispatcher(bool work_in_io)
  : work_in_io_(work_in_io) {
  round_robin_counter_.store(0);
}

base::MessageLoop* WorkLoadDispatcher::GetNextWorkLoop() {
  if (work_loops_.size() && !work_in_io_) {
    uint32_t counter = round_robin_counter_.fetch_add(1);
    int32_t index = counter % work_loops_.size();
    return work_loops_[index];
  }
  return NULL;
}

bool WorkLoadDispatcher::SetWorkContext(WorkContext& ctx) {
  ctx.loop = base::MessageLoop::Current();
  return ctx.loop != NULL;
}

bool WorkLoadDispatcher::SetWorkContext(ProtocolMessage* message) {
  CHECK(message);
  auto& work_context = message->GetWorkCtx();
  work_context.loop = base::MessageLoop::Current();
  return work_context.loop != NULL;
}

bool WorkLoadDispatcher::TransmitToWorker(base::StlClosure& clourse) {
  if (HandleWorkInIOLoop()) {
    clourse();
    return true;
  }
  base::MessageLoop* loop = GetNextWorkLoop();
  if (loop) {
    loop->PostTask(base::NewClosure(clourse));
    return true;
  }
  return false;
}

}//end namespace net
