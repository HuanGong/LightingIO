
#include "coroutine.h"
#include <vector>
#include <iostream>
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include <atomic>
#include <iostream>
#include <mutex>
#include <base/spin_lock.h>
#include <base/base_constants.h>

namespace base {

#define COROSTACKSIZE 1024*16

static base::SpinLock g_coro_lock;

//static
void Coroutine::run_coroutine(void* arg) {
  Coroutine* coroutine = static_cast<Coroutine*>(arg);

  do {
    coroutine->current_state_ = CoroState::kRunning;

    if (coroutine->coro_task_) {
      coroutine->coro_task_();
    }

    coroutine->current_state_ = CoroState::kDone;

    //delete the corotine or reuse it
    CoroScheduler::TlsCurrent()->GcCoroutine(coroutine);
  } while(1);
}

Coroutine::Coroutine(int stack_size, bool main)
  : meta_coro_(main),
    stack_size_(stack_size) {

  if (meta_coro_) {
    current_state_ = CoroState::kRunning;
    coro_create(this, NULL, NULL, NULL, 0);
    return;
  }

  if (stack_size_ < 1024) {
    stack_size_ = COROSTACKSIZE;
  }
  int r = coro_stack_alloc(&stack_, stack_size_);
  LOG_IF(ERROR, r == 0) << "Failed Allocate Coroutine Stack";
  CHECK(r == 1);

  current_state_ = CoroState::kInitialized;
  memset(&coro_ctx_, 0, sizeof(coro_ctx_));
  {
    base::SpinLockGuard guard(g_coro_lock);
    coro_create(&coro_ctx_, &Coroutine::run_coroutine, this, stack_.sptr, stack_.ssze);
  }
}
Coroutine::~Coroutine() {
  //VLOG(GLOG_VTRACE) << "coroutine gone! count:" << count << "st:" << current_state_;
  DCHECK(current_state_ == kDone);
  if (!meta_coro_) {
    coro_stack_free(&stack_);
  }
}

void Coroutine::Reset() {
  static std::function<void()> null_func;
  coro_task_ = null_func;
  current_state_ = CoroState::kInitialized;
}

void Coroutine::SetCoroTask(CoroClosure task) {
  coro_task_ = task;
}

intptr_t Coroutine::Identifier() const {
  return (intptr_t)this;
}

}//end base
