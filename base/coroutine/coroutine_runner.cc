
#include "glog/logging.h"
#include "coroutine_runner.h"
#include "closure/closure_task.h"
#include "memory/lazy_instance.h"

namespace base {
static const int kMaxReuseCoroutineNumbersPerThread = 5000;

namespace __detail {
  struct _T : public CoroRunner {};
}
static thread_local base::LazyInstance<__detail::_T> current_runner = LAZY_INSTANCE_INIT;


//static
CoroRunner& CoroRunner::Runner() {
  return current_runner.get();
}

//static
RefCoroutine CoroRunner::CurrentCoro() {
  return current_runner->current_;
}

bool CoroRunner::CanYield() {
  return !current_runner->InMainCoroutine();
}

void CoroRunner::GcCoroutine() {

  if (free_list_.size() < max_reuse_coroutines_) {

    current_->Reset();
    free_list_.push_back(current_);

  } else {

    current_->ReleaseSelfHolder();
    current_->SetCoroState(CoroState::kDone); 

    expired_coros_.push_back(current_);

    if (!gc_task_scheduled_ || expired_coros_.size() > 100) {
      bind_loop_->PostDelayTask(NewClosure(gc_task_), 1);
      gc_task_scheduled_ = true;
    }
  }

  Transfer(main_coro_);
}

CoroRunner::CoroRunner()
  : gc_task_scheduled_(false),
    bind_loop_(MessageLoop::Current()),
    max_reuse_coroutines_(kMaxReuseCoroutineNumbersPerThread) {

  current_ = main_coro_ = Coroutine::Create(this, true);

  gc_task_ = std::bind(&CoroRunner::ReleaseExpiredCoroutine, this);
  LOG_IF(ERROR, !bind_loop_) << "CoroRunner need constructor with initialized loop";
  CHECK(bind_loop_);
}

CoroRunner::~CoroRunner() {
  expired_coros_.clear();

  current_.reset();
  main_coro_.reset();
}

void CoroRunner::YieldCurrent(int32_t wc) {
  CHECK(!current_runner->InMainCoroutine());
  CHECK(current_runner->current_->wc_ == 0);
  current_runner->current_->wc_ = wc;

  current_runner->Transfer(current_runner->main_coro_);
}

/* 如果本身是在一个子coro里面, 需要在重新将resumecoroutine调度到主coroutine内
 * 不支持嵌套的coroutinetransfer........
 * 如果本身是主coro， 则直接tranfer去执行.
 * 如果不是在调度的线程里， 则posttask去Resume*/
void CoroRunner::ResumeCoroutine(const RefCoroutine& coroutine) {

  if (!bind_loop_->IsInLoopThread() || CanYield()) {
    auto f = std::bind(&CoroRunner::ResumeCoroutine, this, coroutine);
    bind_loop_->PostTask(NewClosure(f));
    return;
  }

  CHECK(InMainCoroutine());

  if (coroutine->Status() != CoroState::kPaused) {
    LOG(ERROR) << " Attempt resume a not paused coroutine id:" << coroutine->identify_
      << " wait counter:" << coroutine->wc_;
    return;
  }

  if (--(coroutine->wc_) > 0) {
    return;
  }
  coroutine->wc_ = 0;
  coroutine->identify_++;
  Transfer(coroutine);
}

bool CoroRunner::Transfer(const RefCoroutine& next) {
  CHECK(current_ != next);

  coro_context* to = next.get();
  coro_context* from = current_.get();
  
  if (current_->state_ != CoroState::kDone) {
    current_->SetCoroState(CoroState::kPaused);
  }
  current_ = next;
  current_->SetCoroState(CoroState::kRunning);
  coro_transfer(from, to);
  return true;
}

void CoroRunner::ReleaseExpiredCoroutine() {
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

void CoroRunner::RunScheduledTasks(std::list<StlClosure>&& tasks) {
  MessageLoop* loop = MessageLoop::Current();
  CHECK(loop == current_runner->bind_loop_);

  current_runner->coro_tasks_.splice(current_runner->coro_tasks_.end(), tasks);
  current_runner->InvokeCoroutineTasks();
}

void CoroRunner::InvokeCoroutineTasks() {

  while (coro_tasks_.size() > 0) {

    RefCoroutine coro = RetrieveCoroutine();
    CHECK(coro);

    coro->SetTask(std::move(coro_tasks_.front()));
    coro_tasks_.pop_front();

    Transfer(coro);

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

RefCoroutine CoroRunner::RetrieveCoroutine() {

  RefCoroutine coro;

  while(free_list_.size()) {
    coro = std::move(free_list_.front());
    free_list_.pop_front();
  }

  if (!coro) { //create new coroutine

    coro = Coroutine::Create(this);
    coro->SelfHolder(coro);

    uint32_t new_size = max_reuse_coroutines_ + 200;
    max_reuse_coroutines_ = new_size > kMaxReuseCoroutineNumbersPerThread ? kMaxReuseCoroutineNumbersPerThread : new_size;
  }
  return std::move(coro);
}

}// end base
