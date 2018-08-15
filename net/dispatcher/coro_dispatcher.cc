#include "glog/logging.h"

#include "tcp_channel.h"
#include "coro_dispatcher.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_task.h"
#include "base/coroutine/coroutine.h"
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

bool CoroWlDispatcher::ResumeWorkContext(WorkContext& ctx) {
  base::RefCoroutine coro = ctx.weak_coro.lock();
  if (!coro || coro->TaskIdentify() != ctx.task_identify) {
    VLOG(GLOG_VINFO) << "Resume Croutine from workcontext failed";
    return false;
  }
  return coro->Resume();
}

bool CoroWlDispatcher::SetWorkContext(WorkContext& ctx) {
  if (base::MessageLoop::Current() &&
      base::CoroScheduler::TlsCurrent()->CanYield()) {
    base::RefCoroutine coro = base::CoroScheduler::CurrentCoro();

    ctx.weak_coro = coro;
    ctx.loop = base::MessageLoop::Current();
    ctx.task_identify = coro->TaskIdentify();
    return true;
  }
  return false;

}

bool CoroWlDispatcher::SetWorkContext(ProtocolMessage* message) {
  CHECK(message);
  if (base::MessageLoop::Current() &&
      base::CoroScheduler::TlsCurrent()->CanYield()) {

    auto& work_context = message->GetWorkCtx();

    work_context.loop = base::MessageLoop::Current();
    base::RefCoroutine coro = base::CoroScheduler::CurrentCoro();
    work_context.weak_coro = coro;
    work_context.task_identify = coro->TaskIdentify();
    return true;
  }
  return false;
}

bool CoroWlDispatcher::TransmitToWorker(base::StlClosure& closure) {
  base::MessageLoop* loop = HandleWorkInIOLoop() ? base::MessageLoop::Current() : GetNextWorkLoop();

  if (NULL == loop) {
    LOG(ERROR) << "no message loop handler this task";
    return false;
  }

  loop->PostCoroTask(closure);
  return true;
}

}//end namespace
