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
  ioloop->PostTask(NewClosure(std::move(clourse)));
  base::CoroRunner::YieldCurrent();
}

bool CoroWlDispatcher::ResumeWorkContext(WorkContext& ctx) {
	if (ctx.resume_ctx) {
	  ctx.resume_ctx();
	}
	return true;
}

bool CoroWlDispatcher::SetWorkContext(WorkContext& ctx) {
  if (base::MessageLoop::Current() &&
      base::CoroRunner::CanYield()) {

    ctx.loop = base::MessageLoop::Current();
    ctx.resume_ctx = base::CoroRunner::CurrentCoroResumeCtx();
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

  if (loop->IsInLoopThread()) {
    go closure;
  } else {
    loop->PostTask(NewClosure([=]() {
      go closure;
    }));
  }
  return true;
}

}//end namespace
