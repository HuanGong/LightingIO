
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

void CoroScheduler::GcCoroutine(Coroutine* die) {
  CHECK(die == current_.get());

  if (free_list_.size() >= max_reuse_coroutines_) {

    current_->ReleaseSelfHolder();
    expired_coros_.push_back(current_);

    if (!gc_task_scheduled_) {
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

  main_coro_ = Coroutine::Create(true);
  current_ = main_coro_;

  gc_task_ = std::bind(&CoroScheduler::ReleaseExpiredCoroutine, this);
  coro_recall_func_ = std::bind(&CoroScheduler::RecallCoroutineIfNeeded, this, std::placeholders::_1);
}

CoroScheduler::~CoroScheduler() {
  expired_coros_.clear();

  current_.reset();
  main_coro_.reset();
}

void CoroScheduler::YieldCurrent() {
  CHECK(main_coro_ != current_);
  Transfer(main_coro_);
}
/* 如果本身是在一个子coro里面, 调用了to->Resume()， 则需要posttask去Resume, 否则再也回不来了
 * 如果本身是主coro， 则直接tranfer去执行.
 * 如果不是在调度的线程里， 则posttask去Resume*/
bool CoroScheduler::ResumeWeakCoroutine(MessageLoop* loop, std::weak_ptr<Coroutine> weak) {
  RefCoroutine coro = weak.lock();
  if (nullptr == coro.get()) {
    LOG(INFO) << __FUNCTION__ << "coroutine can't resume";
    return false;
  }
  if (CanYield() || !loop->IsInLoopThread()) {
    loop->PostTask(base::NewClosure(coro->resume_func_));
    return true;
  }
  Transfer(coro);
  return true;
}

bool CoroScheduler::Transfer(RefCoroutine& next) {
  CHECK(current_ != next);

  next->SetCoroState(CoroState::kRunning);
  //Hack: this caused by GcCorotine
  if (current_->Status() != CoroState::kDone) {
    current_->SetCoroState(CoroState::kPaused);
  }

  coro_context* to = next.get();//&next->coro_ctx_;
  coro_context* from = current_.get();//&current_->coro_ctx_;

  current_ = next;
  coro_transfer(from, to);
  return true;
}

void CoroScheduler::ReleaseExpiredCoroutine() {
  LOG(INFO) << __FUNCTION__ << " release expired coroutine enter, count:" << SystemCoroutineCount();
  expired_coros_.clear();
  gc_task_scheduled_ = false;
  LOG(INFO) << __FUNCTION__ << " release expired coroutine leave, count:" << SystemCoroutineCount();
}
// recall coroutine:
// if has task, set and ran again; else gc this coroutine;
void CoroScheduler::RecallCoroutineIfNeeded(Coroutine* coro) {
  if (coro_tasks_.size() > 0) {
    coro->SetTask(std::move(coro_tasks_.front()));
    coro_tasks_.pop_front();
    return;
  }
  GcCoroutine(coro);
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

      coro = Coroutine::Create();
      coro->SelfHolder(coro);
      coro->recall_callback_ = coro_recall_func_;
      std::weak_ptr<Coroutine> weak_ptr(coro);
      coro->resume_func_ = std::bind(&CoroScheduler::ResumeWeakCoroutine, this, MessageLoop::Current(), weak_ptr);
      // 说明不够用了, 增加复用coro的数量
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
