
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include "closure/closure_task.h"
#include "memory/lazy_instance.h"

namespace base {
static const int kMaxReuseCoroutineNumbersPerThread = 5000;

//static thread safe
CoroScheduler* CoroScheduler::TlsCurrent() {
  //static thread_local CoroScheduler scheduler;
  struct _T : public CoroScheduler {};
  static thread_local base::LazyInstance<_T> scheduler = LAZY_INSTANCE_INIT;
  return scheduler.ptr();
}
//static
RefCoroutine CoroScheduler::CurrentCoro() {
  return TlsCurrent()->current_;
}

bool CoroScheduler::CanYield() const {
  return current_ && main_coro_ != current_;
}

void CoroScheduler::GcCoroutine() {

  if (free_list_.size() >= max_reuse_coroutines_) {

    current_->ReleaseSelfHolder();
    expired_coros_.push_back(current_);

    if (!gc_task_scheduled_ || expired_coros_.size() > 100) {
      LOG(INFO) << " schedule a gc task to free coroutine:";
      MessageLoop::Current()->PostDelayTask(NewClosure(gc_task_), 1);
      gc_task_scheduled_ = true;
    }

  } else {
    free_list_.push_back(current_);
  }

  Transfer(main_coro_);
}

CoroScheduler::CoroScheduler()
  : gc_task_scheduled_(false),
    max_reuse_coroutines_(kMaxReuseCoroutineNumbersPerThread) {

  main_coro_ = Coroutine::Create(this, true);
  current_ = main_coro_;

  gc_task_ = std::bind(&CoroScheduler::ReleaseExpiredCoroutine, this);
}

CoroScheduler::~CoroScheduler() {
  expired_coros_.clear();

  current_.reset();
  main_coro_.reset();
}

void CoroScheduler::YieldCurrent(int32_t wc) {
  CHECK(main_coro_ != current_);
  CHECK(current_->wc_ == 0);
  current_->wc_ = wc;
  Transfer(main_coro_);
}
/* 如果本身是在一个子coro里面, 调用了to->Resume()， 则需要posttask去Resume, 否则再也回不来了
 * 如果本身是主coro， 则直接tranfer去执行.
 * 如果不是在调度的线程里， 则posttask去Resume*/
bool CoroScheduler::ResumeCoroutine(MessageLoop* loop, std::weak_ptr<Coroutine> weak) {
  RefCoroutine coro = weak.lock();
  CHECK(coro);

  if (coro->Status() != CoroState::kPaused) {
    LOG(INFO) << __FUNCTION__ << "can't resume a none kPaused coroutine";
    return false;
  }

  if (CanYield() || !loop->IsInLoopThread()) { //make sure run inloop and in mian_coro
    loop->PostTask(base::NewClosure(coro->resume_func_));
    return true;
  }

  {
    CHECK(current_ == main_coro_);
    if (coro->Status() != CoroState::kPaused) {
      LOG(INFO) << __FUNCTION__ << "can't resume a none kPaused coroutine";
      return false;
    }
    if (--(coro->wc_) > 0) {
      return false;
    }
    LOG(INFO) << " resume coroutine:" << coro.get() << " wait_count:" << coro->wc_;
    coro->wc_ = 0;
    Transfer(coro);
  }

  return true;
}

bool CoroScheduler::Transfer(RefCoroutine& next) {
  CHECK(current_ != next);

  coro_context* to = next.get();//&next->coro_ctx_;
  coro_context* from = current_.get();//&current_->coro_ctx_;

  current_->SetCoroState(CoroState::kPaused);
  current_ = next;
  current_->SetCoroState(CoroState::kRunning);
  coro_transfer(from, to);
  return true;
}

void CoroScheduler::ReleaseExpiredCoroutine() {
  LOG(INFO) << __FUNCTION__ << " release expired coroutine enter, count:" << SystemCoroutineCount();
  expired_coros_.clear();
  gc_task_scheduled_ = false;
  LOG(INFO) << __FUNCTION__ << " release expired coroutine leave, count:" << SystemCoroutineCount();
}

void CoroScheduler::RecallCoroutineIfNeeded() {
  CHECK(current_ != main_coro_);

  current_->Reset();

  if (coro_tasks_.size() > 0) {
    current_->SetTask(std::move(coro_tasks_.front()));
    coro_tasks_.pop_front();
    current_->SetCoroState(CoroState::kRunning);
    return;
  }
  GcCoroutine();
}

void CoroScheduler::RunScheduledTasks(std::list<StlClosure>&& tasks) {
  MessageLoop* loop = MessageLoop::Current();
  CHECK(loop);

  coro_tasks_.splice(coro_tasks_.end(), tasks);
  while (coro_tasks_.size() > 0) {

    RefCoroutine coro;
    while(!coro && free_list_.size()) {
      coro = free_list_.front();
      free_list_.pop_front();
    }

    if (!coro) { //create new coroutine

      coro = Coroutine::Create(this);
      coro->SelfHolder(coro);

      std::weak_ptr<Coroutine> weak(coro);
      coro->resume_func_ = std::bind(&CoroScheduler::ResumeCoroutine, this, loop, weak);
      uint32_t new_size = max_reuse_coroutines_ + 200;
      max_reuse_coroutines_ = new_size > kMaxReuseCoroutineNumbersPerThread ? kMaxReuseCoroutineNumbersPerThread : new_size;
    }

    coro->SetTask(std::move(coro_tasks_.front()));
    coro_tasks_.pop_front();

    TlsCurrent()->Transfer(coro);

    // 只有当这个corotine 在task运行的过程中yield了， 那么才会回到这里,否则是在task运行完成之后
    // 会调用RecallCoroutineIfNeeded，在依旧有task需要运行的时候，可以设置task， 让这个coro继续
    // 执行task，而不需要切换task，如果没有task， 则将coro回收或者标记成gc状态，等待gc回收
    //
    // 当运行的task在一半的状态被yield切换出去，回到这里， 如果仍然有task需要调度， 则找一个新的
    // coro运行task，结束循环回到messageloop循环，等待后后续的Task
    //
    // 当yield的之后的task重新被标记成可运行，那么做coro的切换就无法避免了，直接使用resumecoroutine
    // 切换调度, 此时调度结束后调用RecallCoroutineIfNeeded，task队列中应该会是空的
  }
}

}// end base
