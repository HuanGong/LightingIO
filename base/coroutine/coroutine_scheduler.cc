
#include "coroutine_scheduler.h"
#include "message_loop/closure_task.h"

namespace base {

CoroScheduler::CoroScheduler(MessageLoop* loop)
  : tls_main_coro_(nullptr),
    schedule_loop_(loop) {

}
CoroScheduler::~CoroScheduler() {
  tls_main_coro_ = nullptr;
  schedule_loop_ = nullptr;
}

void CoroScheduler::ScheduleCoro(Coroutine* coro) {
  schedule_loop_->PostTask(NewClosure(std::bind(&ScheduleCoro::schedule_coro, this, coro)));
}

void CoroScheduler::schedule_coro(Coroutine* coro) {
  tls_main_coro_->Transfer(coro);
}

}// end base
