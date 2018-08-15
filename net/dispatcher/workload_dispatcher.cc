
#include "glog/logging.h"
#include "workload_dispatcher.h"

#include "net/protocol/proto_message.h"

namespace net {

WorkLoadDispatcher::WorkLoadDispatcher(bool work_in_io)
  : work_in_io_(work_in_io) {
  round_robin_counter_.store(0);
}

void WorkLoadDispatcher::InitWorkLoop(const int32_t count) {
  if (work_in_io_) {
    LOG(ERROR) << "Don't Need WorkerLoop When Handle Work in IOLoop";
    return;
  }

  for (int32_t i = 0; i < count; i++) {
    const static std::string name_prefix("WorkLoop:");
    RefMessageLoop loop(new base::MessageLoop);
    loop->SetLoopName(name_prefix + std::to_string(i));
    work_loops_.push_back(std::move(loop));
  }
  LOG(INFO) << "WorkLoadDispatcher Create [" << work_loops_.size() << "] loops Handle Work";
}

void WorkLoadDispatcher::StartDispatcher() {
  for (auto& loop : work_loops_) {
    loop->Start();
  }
}

base::MessageLoop* WorkLoadDispatcher::GetNextWorkLoop() {
  if (work_loops_.size() && !work_in_io_) {
    int32_t index = round_robin_counter_ % work_loops_.size();
    round_robin_counter_++;
    return work_loops_[index].get();
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
