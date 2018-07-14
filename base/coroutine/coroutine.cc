
#include "coroutine.h"
#include <vector>
#include <iostream>
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include <atomic>
#include <iostream>
#include <mutex>
#include <base/spin_lock.h>
#include <base/message_loop/message_loop.h>

namespace base {

#define COROSTACKSIZE 1024*16

static std::mutex g_coro_mutex;
static base::SpinLock g_coro_lock;

//static
void Coroutine::run_coroutine(void* arg) {

  Coroutine* coroutine = static_cast<Coroutine*>(arg);

  coroutine->current_state_ = CoroState::kRunning;

  coroutine->RunCoroTask();

  coroutine->current_state_ = CoroState::kDone;
  CoroScheduler::TlsCurrent()->GcCoroutine(coroutine);
}

Coroutine::Coroutine(int stack_size, bool main)
  : stack_size_(stack_size),
    meta_coro_(main) {

  if (meta_coro_) {

    current_state_ = CoroState::kRunning;
    coro_create(this, NULL, NULL, NULL, 0);

  } else {
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
      //std::unique_lock<std::mutex> lck(g_coro_mutex);
      coro_create(&coro_ctx_, &Coroutine::run_coroutine, this, stack_.sptr, stack_.ssze);
    }
  }
}

Coroutine::~Coroutine() {
  //count--;
  //LOG(INFO) << "coroutine gone! count:" << count << "st:" << current_state_;
  CHECK(current_state_ == kDone);
  if (!meta_coro_) {
    coro_stack_free(&stack_);
  }
}

void Coroutine::RunCoroTask() {
  if (coro_task_) {
    coro_task_();
  }
}

void Coroutine::SetCoroTask(CoroClosure task) {
  coro_task_ = task;
}

intptr_t Coroutine::Identifier() const {
  return (intptr_t)this;
}

}//end base
