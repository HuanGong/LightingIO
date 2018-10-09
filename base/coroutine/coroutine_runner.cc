
#include "glog/logging.h"
#include "coroutine_runner.h"
#include "closure/closure_task.h"
#include "memory/lazy_instance.h"

namespace base {
static const int kMaxReuseCoroutineNumbersPerThread = 500;

namespace __detail {
  struct _T : public CoroRunner {};
}
static thread_local base::LazyInstance<__detail::_T> tls_runner = LAZY_INSTANCE_INIT;

bool CoroRunner::CanYield() {
  return !tls_runner->InMainCoroutine();
}

void CoroRunner::GcCoroutine() {
  CHECK(main_coro_ != current_);

  if (free_list_.size() < max_reuse_coroutines_) {

    current_->Reset();
    free_list_.push_back(current_);

  } else {
    current_->SetCoroState(CoroState::kDone);
    expired_coros_.push_back(current_);

    if (!gc_task_scheduled_ || expired_coros_.size() > 100) {
      bind_loop_->PostDelayTask(NewClosure(gc_task_), 1);
      gc_task_scheduled_ = true;
    }
  }

  TransferTo(main_coro_);
}

CoroRunner::CoroRunner()
  : gc_task_scheduled_(false),
    bind_loop_(MessageLoop::Current()),
    max_reuse_coroutines_(kMaxReuseCoroutineNumbersPerThread) {

  RefCoroutine coro_ptr = Coroutine::Create(this, true);
  coro_ptr->SelfHolder(coro_ptr);
  current_ = main_coro_ = coro_ptr.get();

  gc_task_ = std::bind(&CoroRunner::ReleaseExpiredCoroutine, this);
  LOG_IF(ERROR, !bind_loop_) << "CoroRunner need constructor with initialized loop";
  CHECK(bind_loop_);
}

CoroRunner::~CoroRunner() {
  ReleaseExpiredCoroutine();
  current_->ReleaseSelfHolder();
  main_coro_->ReleaseSelfHolder();
  current_ = nullptr;
  main_coro_ = nullptr;
}

CoroRunner& CoroRunner::Runner() {
  return tls_runner.get();
}

void CoroRunner::YieldCurrent(int32_t wc) {
  CHECK(tls_runner->current_->wc_ == 0);

  if (tls_runner->InMainCoroutine()) {
    LOG(INFO) << " You try to going pause a coroutine which can't be yield";
    return;
  }
  tls_runner->current_->wc_ = wc;
  tls_runner->TransferTo(tls_runner->main_coro_);
}

/* 如果本身是在一个子coro里面, 需要在重新将resumecoroutine调度到主coroutine内
 * 不支持嵌套的coroutinetransfer........
 * 如果本身是主coro， 则直接tranfer去执行.
 * 如果不是在调度的线程里， 则posttask去Resume*/
void CoroRunner::ResumeCoroutine(std::weak_ptr<Coroutine> weak, uint64_t id) {

  if (!bind_loop_->IsInLoopThread() || CanYield()) {
    auto f = std::bind(&CoroRunner::ResumeCoroutine, this, weak, id);
    bind_loop_->PostTask(NewClosure(f));
    return;
  }

  CHECK(InMainCoroutine());
  Coroutine* coroutine = nullptr;
  {
    RefCoroutine coro = weak.lock();
    if (!coro) {
      LOG(ERROR) << " coroutine has gone";
      return;
    }
    coroutine = coro.get();
  }

  if (coroutine->Status() != CoroState::kPaused || coroutine->identify_ != id) {
    LOG(ERROR) << " can't resume this coroutine, id:" << coroutine->identify_
               << " wait counter:" << coroutine->wc_;
    return;
  }

  if (--(coroutine->wc_) > 0) {
    return;
  }

  coroutine->wc_ = 0;
  coroutine->identify_++;
  TransferTo(coroutine);
}

StlClosure CoroRunner::CurrentCoroResumeCtx() {
  return std::bind(&CoroRunner::ResumeCoroutine,
                   tls_runner.ptr(),
                   tls_runner->current_->AsWeakPtr(),
                   tls_runner->current_->identify_);
}

void CoroRunner::TransferTo(Coroutine *next) {
  CHECK(current_ != next);

  coro_context* to = next;
  coro_context* from = current_;
  if (current_->state_ != CoroState::kDone) {
    current_->SetCoroState(CoroState::kPaused);
  }
  current_ = next;
  current_->SetCoroState(CoroState::kRunning);
  coro_transfer(from, to);
}

void CoroRunner::ReleaseExpiredCoroutine() {
  for (auto& coro : expired_coros_) {
    coro->ReleaseSelfHolder();
  }
  expired_coros_.clear();
  gc_task_scheduled_ = false;
}

void CoroRunner::RecallCoroutineIfNeeded() {
  CHECK(current_ != main_coro_);

  if (coro_tasks_.size() > 0) {
    current_->SetTask(std::move(coro_tasks_.front()));
    coro_tasks_.pop_front();
    current_->SetCoroState(CoroState::kRunning);
    return;
  }
  GcCoroutine();
}

void CoroRunner::InvokeCoroutineTasks() {

  while (coro_tasks_.size() > 0) {

    Coroutine* coro = RetrieveCoroutine();
    CHECK(coro);

    coro->SetTask(std::move(coro_tasks_.front()));
    coro_tasks_.pop_front();

    TransferTo(coro);

    // 只有当这个corotine 在task运行的过程中换出(paused)，那么才会重新回到这里,否则是在task运行完成之后
    // 会调用RecallCoroutine，在依旧有task需要运行的时候，可以设置task， 让这个coro继续
    // 执行task，而不需要切换task，如果没有task， 则将coro回收或者标记成gc状态，等待gc回收
    //
    // 当运行的task在一半的状态被切换出去，回到这里， 如果仍然有task需要调度， 则找一个新的
    // coro运行task，结束循环回到messageloop循环，等待后后续的Task
    //
    // 当yield的之后的task重新被标记成可运行，那么做coro的切换就无法避免了，直接使用resumecoroutine
    // 切换调度, 此时调度结束后调用RecallCoroutineIfNeeded，task队列中应该会是空的
  }
  invoke_coro_shceduled_ = false;
}

Coroutine* CoroRunner::RetrieveCoroutine() {

  Coroutine* coro = nullptr;
  while(free_list_.size() && !coro) {
    coro = free_list_.front();
    free_list_.pop_front();
  }

  if (nullptr == coro) { //create new coroutine
    auto coro_ptr = Coroutine::Create(this);
    coro_ptr->SelfHolder(coro_ptr);
    coro = coro_ptr.get();
  }
  return coro;
}

}// end base
