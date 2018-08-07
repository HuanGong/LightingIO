

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

void CoroWlDispatcher::TransferAndYield(base::MessageLoop* ioloop, base::StlClourse clourse) {
  CHECK(ioloop);
  CHECK(base::CoroScheduler::TlsCurrent()->CurrentCoro());
  ioloop->PostTask(base::NewClosure(std::move(clourse)));

  base::CoroScheduler::TlsCurrent()->YieldCurrent();
}

bool CoroWlDispatcher::ResumeWorkCtxForRequest(RefProtocolMessage& request) {

  auto& work_context = request->GetWorkCtx();
  if (!work_context.coro_loop || !work_context.weak_coro.lock()) {
    LOG(FATAL) << " This Request WorkContext Is Not Init";
    return false;
  }

  base::StlClourse functor = [=]() {
    auto& work_context = request->GetWorkCtx();
    base::RefCoroutine coro = work_context.weak_coro.lock();
    if (coro && coro->Status() == base::CoroState::kPaused) {
      base::CoroScheduler::TlsCurrent()->ResumeCoroutine(coro);
    }
  };
  work_context.coro_loop->PostTask(base::NewClosure(std::move(functor)));
  return true;
}

bool CoroWlDispatcher::SetWorkContext(ProtocolMessage* message) {
  CHECK(message);
  if ((base::MessageLoop::Current() &&
      base::CoroScheduler::TlsCurrent()->CurrentCoro() &&
      !base::CoroScheduler::TlsCurrent()->InRootCoroutine())) {
    auto& work_context = message->GetWorkCtx();
    work_context.coro_loop = base::MessageLoop::Current();
    work_context.weak_coro = base::CoroScheduler::TlsCurrent()->CurrentCoro();
    return true;
  }
  return false;
}

bool CoroWlDispatcher::TransmitToWorker(base::StlClourse& closure) {

  if (HandleWorkInIOLoop()) {
    base::CoroScheduler::CreateAndTransfer(closure);
    return true;
  }

  base::MessageLoop* loop = GetNextWorkLoop();
  if (NULL == loop) {
    return false;
  }
  base::CoroScheduler::ScheduleCoroutineInLoop(loop, closure);
  return true;
}

}//end namespace
