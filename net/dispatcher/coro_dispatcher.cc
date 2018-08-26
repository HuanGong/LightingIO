#include "glog/logging.h"

#include "tcp_channel.h"
#include "coro_dispatcher.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_task.h"
#include "base/coroutine/coroutine.h"
#include "base/coroutine/coroutine_runner.h"

namespace net {

CoroWlDispatcher::CoroWlDispatcher(bool handle_in_io)
  : WorkLoadDispatcher(handle_in_io) {
}

CoroWlDispatcher::~CoroWlDispatcher() {
}

void CoroWlDispatcher::TransferAndYield(base::MessageLoop* ioloop, base::StlClosure clourse) {
  CHECK(ioloop);
  CHECK(base::CoroRunner::CurrentCoro());
  ioloop->PostTask(base::NewClosure(std::move(clourse)));

  base::CoroRunner::YieldCurrent();
}

bool CoroWlDispatcher::ResumeWorkContext(WorkContext& ctx) {
  base::RefCoroutine coro = ctx.weak_coro.lock();
  if (!coro || coro->Identify() != ctx.task_identify) {
    VLOG(GLOG_VINFO) << "Resume Croutine from workcontext failed";
    return false;
  }
  coro->Resume(ctx.task_identify);
  return true;
}

bool CoroWlDispatcher::SetWorkContext(WorkContext& ctx) {
  if (base::MessageLoop::Current() &&
      base::CoroRunner::CanYield()) {

    base::RefCoroutine coro = base::CoroRunner::CurrentCoro();

    ctx.weak_coro = coro;
    ctx.loop = base::MessageLoop::Current();
    ctx.task_identify = coro->Identify();
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
