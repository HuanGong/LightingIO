
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

//IMPORTANT: NO HEAP MEMORY HERE!!!
void coro_main(void* arg) {
  Coroutine* coroutine = static_cast<Coroutine*>(arg);
  CHECK(coroutine);
  do {
    if (coroutine->start_callback_) {
      coroutine->start_callback_(coroutine);
    }
    coroutine->SetCoroState(CoroState::kRunning);
    CHECK(coroutine->coro_task_);
    coroutine->coro_task_();
    coroutine->coro_task_ = null_func;
    coroutine->SetCoroState(CoroState::kDone);

    CHECK(coroutine->recall_callback_);
    coroutine->recall_callback_(coroutine);
  } while(true);
}

//static
int64_t SystemCoroutineCount() {
  return coroutine_counter.load();
}
//static
std::shared_ptr<Coroutine> Coroutine::Create(bool main) {
  return std::shared_ptr<Coroutine>(new Coroutine(main));
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
    coro_create(this, coro_main, this, stack_.sptr, stack_.ssze);
  }
  LOG(INFO) << "Coroutine Born +1";
  coroutine_counter.fetch_add(1);
}

Coroutine::~Coroutine() {
  coroutine_counter.fetch_sub(1);
  DCHECK(current_state_ != kPaused);
  LOG(INFO) << "Coroutine Gone -1, now has" << coroutine_counter.load();
  VLOG(GLOG_VTRACE) << "coroutine gone! count:" << coroutine_counter.load() << "st:" << StateToString();

  if (stack_.ssze != 0) {
    coro_stack_free(&stack_);
  }
}

void Coroutine::SelfHolder(RefCoroutine& self) {
  CHECK(self.get() == this);
  self_holder_ = self;
}

void Coroutine::Reset() {
  coro_task_ = null_func;
  current_state_ = CoroState::kInitialized;
}

bool Coroutine::Resume() {
  if (resume_func_ && current_state_ == CoroState::kPaused) {
    resume_func_();
    return true;
  }
  LOG_IF(ERROR, !resume_func_) << "resume-function is empty, can't resume this coro";
  LOG_IF(ERROR, (current_state_ != CoroState::kPaused)) << "Can't resume a coroutine its paused, state:" << StateToString();
  return false;
}

std::string Coroutine::StateToString() const {
  switch(current_state_) {
    case CoroState::kInitialized:
      return "Initialized";
    case CoroState::kRunning:
      return "Running";
    case CoroState::kPaused:
      return "Paused";
    case CoroState::kDone:
      return "Done";
    default:
      return "Unknown";
  }
  return "Unknown";
}

}//end base
