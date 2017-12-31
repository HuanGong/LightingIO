

#include "glog/logging.h"

#include "coro_dispatcher.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"

#include "base/coroutine/coroutine_task.h"
#include "base/coroutine/coroutine_scheduler.h"

namespace net {

CoroWlDispatcher::CoroWlDispatcher()
  : WorkLoadDispatcher() {

}

CoroWlDispatcher::~CoroWlDispatcher() {

}

bool CoroWlDispatcher::Dispatch(ProtoMessageHandler h, RefProtocolMessage m) {

  LOG(INFO) << "Going Dispatch work";
  if (!h || !m) {
    return false;
  }
  //h(m);
  auto functor = std::bind(h, m);
  base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(std::move(functor)));
  LOG(INFO) << "Dispatch Finished work be done";
  return true;
}

}//end namespace
