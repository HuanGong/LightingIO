#include "glog/logging.h"

#include "tcp_channel.h"
#include "coro_dispatcher.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_task.h"
#include "base/coroutine/coroutine_scheduler.h"

namespace net {

CoroWlDispatcher::CoroWlDispatcher(bool handle_in_io)
  : WorkLoadDispatcher(handle_in_io) {
}

CoroWlDispatcher::~CoroWlDispatcher() {
}

void CoroWlDispatcher::TransferAndYield(base::MessageLoop* ioloop, base::StlClosure clourse) {
  CHECK(ioloop);
  CHECK(base::CoroScheduler::CurrentCoro());
  ioloop->PostTask(base::NewClosure(std::move(clourse)));

  base::CoroScheduler::TlsCurrent()->YieldCurrent();
}

bool CoroWlDispatcher::ResumeWorkCtxForRequest(RefProtocolMessage& request) {

  auto& work_context = request->GetWorkCtx();
  base::RefCoroutine coro = work_context.weak_coro.lock();
  if (!work_context.coro_loop || !coro) {
    LOG(FATAL) << "No corotine context or coroutine has gone.";
    return false;
  }
  return coro->Resume();
}

bool CoroWlDispatcher::SetWorkContext(ProtocolMessage* message) {
  CHECK(message);
  if (base::MessageLoop::Current() &&
      base::CoroScheduler::TlsCurrent()->CanYield()) {
    auto& work_context = message->GetWorkCtx();
    work_context.coro_loop = base::MessageLoop::Current();
    work_context.weak_coro = base::CoroScheduler::CurrentCoro();
    return true;
  }
  return false;
}

bool CoroWlDispatcher::TransmitToWorker(base::StlClosure& closure) {

  if (HandleWorkInIOLoop()) {
    base::MessageLoop::Current()->PostCoroTask(closure);
    return true;
  }

  base::MessageLoop* loop = GetNextWorkLoop();
  if (NULL == loop) {
    LOG(ERROR) << "no message loop handler this task";
    return false;
  }
  loop->PostCoroTask(closure);
  return true;
}

}//end namespace
