
#include "workload_dispatcher.h"

#include "glog/logging.h"
#include <net_io/codec/codec_message.h>

namespace lt {
namespace net {

Dispatcher::Dispatcher(bool handle_in_io)
  : handle_in_io_(handle_in_io) {
  round_robin_counter_.store(0);
}

base::MessageLoop* Dispatcher::NextWorker() {
  if (workers_.empty()) {
    return NULL;
  }
  uint32_t index = ++round_robin_counter_ % workers_.size();
  return workers_[index];
}

bool Dispatcher::SetWorkContext(CodecMessage* message) {
  auto loop = base::MessageLoop::Current();
  message->SetWorkerCtx(loop);
  return loop != NULL;
}

bool Dispatcher::Dispatch(StlClosure clourse) {
  if (handle_in_io_) {
    clourse();
    return true;
  }

  base::MessageLoop* loop = NextWorker();
  if (nullptr == loop) {
    return false;
  }
  return loop->PostTask(FROM_HERE, clourse);
}

}}//end namespace net
