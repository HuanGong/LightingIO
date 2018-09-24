
#include "coroutine.h"
#include <vector>
#include <iostream>
#include "glog/logging.h"
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
  auto * coroutine = static_cast<Coroutine*>(arg);
  CHECK(coroutine);
  do {
    CHECK(coroutine->coro_task_ && CoroState::kRunning == coroutine->state_);

    coroutine->coro_task_();

    coroutine->delegate_->RecallCoroutineIfNeeded();
  } while(true);
}

//static
int64_t SystemCoroutineCount() {
  return coroutine_counter.load();
}
//static
std::shared_ptr<Coroutine> Coroutine::Create(CoroDelegate* d, bool main) {
  return std::shared_ptr<Coroutine>(new Coroutine(d, main));
}

Coroutine::Coroutine(CoroDelegate* d, bool main) :
  wc_(0),
  delegate_(d),
  identify_(0) {

  stack_.ssze = 0;
  stack_.sptr = nullptr;

  state_ = CoroState::kInitialized;
  if (main) {
    {
      base::SpinLockGuard guard(g_coro_lock);
      coro_create(this, NULL, NULL, NULL, 0);
    }
    state_ = CoroState::kRunning;
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
  coroutine_counter.fetch_add(1);
  LOG(INFO) << "Coroutine Born +1";
}

Coroutine::~Coroutine() {
  coroutine_counter.fetch_sub(1);
  LOG(INFO) << "Coroutine Gone" << " state:" << StateToString() << " now has" << coroutine_counter.load();

  CHECK(state_ == CoroState::kDone);
  VLOG(GLOG_VTRACE) << "coroutine gone! count:" << coroutine_counter.load() << "st:" << StateToString();

  if (stack_.ssze != 0) {
    coro_stack_free(&stack_);
  }
}

void Coroutine::SetTask(CoroClosure task) {
  coro_task_ = task;
};

void Coroutine::SelfHolder(RefCoroutine& self) {
  CHECK(self.get() == this);
  self_holder_ = self;
}

void Coroutine::Reset() {
  wc_ = 0;
  coro_task_ = null_func;
  state_ = CoroState::kInitialized;
}

void Coroutine::Resume(uint64_t id) {
  LOG_IF(ERROR, (state_ != CoroState::kPaused)) << "Can't resume a coroutine not paused, state:" << StateToString()
    << " resume identify:" << id << " current identify:" << identify_;
  delegate_->ResumeCoroutine(shared_from_this());
}

std::string Coroutine::StateToString() const {
  switch(state_) {
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
