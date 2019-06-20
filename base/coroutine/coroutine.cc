#include <vector>
#include <atomic>
#include <mutex>
#include "glog/logging.h"
#include <memory/spin_lock.h>
#include <base/base_constants.h>

#include "coroutine.h"

namespace base {

static base::SpinLock g_coro_lock;
static std::atomic_int64_t coroutine_counter;

//IMPORTANT: NO HEAP MEMORY HERE!!!
void coro_main(void* arg) {
  auto * coroutine = static_cast<Coroutine*>(arg);
  CHECK(coroutine);

  do {
    DCHECK(coroutine->coro_task_ && CoroState::kRunning == coroutine->state_);

    coroutine->coro_task_->Run();
    coroutine->coro_task_.reset();

    coroutine->delegate_->RecallCoroutineIfNeeded();
  } while(coroutine);
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

  memset((coro_context*)this, 0, sizeof(coro_context));
  {
    base::SpinLockGuard guard(g_coro_lock);
    coro_create(this, coro_main, this, stack_.sptr, stack_.ssze);
  }
  coroutine_counter.fetch_add(1);
  VLOG(GLOG_VTRACE) << "coroutine born! count:" << coroutine_counter.load() << "st:" << StateToString();
}

Coroutine::~Coroutine() {
  if (stack_.ssze != 0) { //none main coroutine
    coro_stack_free(&stack_);
    coroutine_counter.fetch_sub(1);
    CHECK(state_ != CoroState::kRunning);
  }

  VLOG(GLOG_VTRACE) << "coroutine gone! count:" << coroutine_counter.load() << "st:" << StateToString();
}

void Coroutine::SetTask(TaskBasePtr&& task) {
  coro_task_ = std::move(task);
};

void Coroutine::SelfHolder(RefCoroutine& self) {
  CHECK(self.get() == this);
  self_holder_ = self;
}

void Coroutine::Reset() {
  coro_task_.reset();
  state_ = CoroState::kInitialized;
}

std::string Coroutine::StateToString() const {
  switch(state_) {
    case CoroState::kDone:
      return "Done";
    case CoroState::kPaused:
      return "Paused";
    case CoroState::kRunning:
      return "Running";
    case CoroState::kInitialized:
      return "Initialized";
    default:
      break;
  }
  return "Unknown";
}

}//end base
