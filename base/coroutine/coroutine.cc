
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

static base::SpinLock g_coro_lock;
static std::atomic_int64_t coroutine_counter;
const static std::function<void()> null_func;

//static
int64_t Coroutine::SystemCoroutineCount() {
  return coroutine_counter.load();
}
//static
std::shared_ptr<Coroutine> Coroutine::Create(bool main) {
  return std::shared_ptr<Coroutine>(new Coroutine(main));
}
//static Do Not Create Heap Memory Here
void Coroutine::run_coroutine(void* arg) {
  Coroutine* coroutine = static_cast<Coroutine*>(arg);
  do {
    if (coroutine->start_callback_) {
      coroutine->start_callback_(coroutine);
    }
    coroutine->SetCoroState(CoroState::kRunning);
    coroutine->RunCoroutineTask();
    coroutine->SetCoroState(CoroState::kDone);

    CHECK(coroutine->recall_callback_);
    coroutine->recall_callback_(coroutine);
  } while(1);
}

Coroutine::Coroutine(bool main) {

  stack_.ssze = 0;
  stack_.sptr = nullptr;

  current_state_ = CoroState::kInitialized;
  if (main) {
    {
      base::SpinLockGuard guard(g_coro_lock);
      coro_create(this, NULL, NULL, NULL, 0);
    }
    current_state_ = CoroState::kRunning;
    return;
  }

  int r = coro_stack_alloc(&stack_, COROUTINE_STACK_SIZE);
  LOG_IF(ERROR, r == 0) << "Failed Allocate Coroutine Stack";
  CHECK(r == 1);

  memset(this, 0, sizeof(coro_context));
  {
    base::SpinLockGuard guard(g_coro_lock);
    coro_create(this, &Coroutine::run_coroutine, this, stack_.sptr, stack_.ssze);
  }
  coroutine_counter.fetch_add(1);
}
Coroutine::~Coroutine() {
  coroutine_counter.fetch_sub(1);
  DCHECK(current_state_ == kDone);

  if (stack_.ssze != 0) {
    coro_stack_free(&stack_);
  }
  VLOG(GLOG_VTRACE) << "coroutine gone! count:" << coroutine_counter.load() << "st:" << current_state_;
}

void Coroutine::Reset() {
  coro_task_ = null_func;
  current_state_ = CoroState::kInitialized;
}

void Coroutine::SetCoroTask(CoroClosure task) {
  coro_task_ = task;
}

void Coroutine::RunCoroutineTask() {
  if (coro_task_) {
    coro_task_();
    coro_task_ = null_func;
  } else {
    LOG(ERROR) << " Empty Coroutine Run!";
  }
}

}//end base
