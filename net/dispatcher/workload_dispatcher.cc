
#include "glog/logging.h"
#include "workload_dispatcher.h"

namespace net {

WorkLoadDispatcher::WorkLoadDispatcher(bool work_in_io)
  : work_in_io_(work_in_io) {
  round_robin_counter_.store(0);
}

void WorkLoadDispatcher::InitWorkLoop(const int32_t count) {
  if (work_in_io_) {
    LOG(ERROR) << "Don't Need WorkLoop When Handle Work in IOLoop";
    return;
  }

  for (int32_t i = 0; i < count; i++) {
    const static std::string name_prefix("WorkLoop:");

    RefMessageLoop loop(new base::MessageLoop2);
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

base::MessageLoop2* WorkLoadDispatcher::GetNextWorkLoop() {
  if (work_loops_.size() && !work_in_io_) {
    int32_t index = round_robin_counter_ % work_loops_.size();
    round_robin_counter_++;
    return work_loops_[index].get();
  }
  return NULL;
}

}//end namespace net
