
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include "closure/closure_task.h"
#include "memory/lazy_instance.h"

namespace base {
static const int kDefaultMaxReuseCoroutineNumbersPerThread = 10000;

//static thread safe
CoroScheduler* CoroScheduler::TlsCurrent() {
  //static thread_local base::LazyInstance<CoroScheduler> scheduler = LAZY_INSTANCE_INIT;
  static thread_local CoroScheduler scheduler;
  return &scheduler;
}

//static
void CoroScheduler::ScheduleCoroutineInLoop(base::MessageLoop* target_loop, StlClosure& t) {
  CHECK(target_loop && t);

  StlClosure coro_functor = std::bind(&CoroScheduler::CreateAndTransfer, t);
  target_loop->PostTask(base::NewClosure(coro_functor));
}

//static
void CoroScheduler::CreateAndTransfer(CoroClosure& task) {
  CHECK(TlsCurrent()->InRootCoroutine());

  base::MessageLoop* loop =  base::MessageLoop::Current();
  CoroScheduler* scheduler = TlsCurrent();
  CHECK(loop);

  RefCoroutine coro;
  while(!coro && scheduler->free_list_.size() > 0) {
    coro = scheduler->free_list_.front();
    scheduler->free_list_.pop_front();
  }

  if (!coro) { //really create new coroutine
    coro = std::make_shared<Coroutine>(0, false);
    scheduler->OnNewCoroBorn(coro);
    coro->resume_func_ = std::bind([=](std::weak_ptr<Coroutine> weak_coro) {
      RefCoroutine to = weak_coro.lock();
      LOG_IF(ERROR, !to) << "Can't Resume a Coroutine Has Gone";
      if (!to) return;

      if (!loop->IsInLoopThread()) {
        loop->PostTask(base::NewClosure(to->resume_func_));
        return;
      }
      TlsCurrent()->Transfer(to);
    }, coro);
  }

  coro->SetCoroTask(std::move(task));

  TlsCurrent()->Transfer(coro);
}

void CoroScheduler::OnNewCoroBorn(RefCoroutine& coro) {
  coroutines_.insert(coro);
}

void CoroScheduler::GcCoroutine(Coroutine* die) {
  CHECK(die == current_.get());

  if (free_list_.size() >= max_reuse_coroutines_) {
    coroutines_.erase(current_);
    expired_coros_.push_back(current_);
    expired_coros_.size() > 100 ? Transfer(gc_coro_) : Transfer(main_coro_);
    return;
  }

  free_list_.push_back(current_);
  Transfer(main_coro_);
}

CoroScheduler::CoroScheduler()
  : max_reuse_coroutines_(kDefaultMaxReuseCoroutineNumbersPerThread) {

  main_coro_.reset(new Coroutine(0, true));
  current_ = main_coro_;

  gc_coro_.reset(new Coroutine(0, false));
  gc_coro_->SetCoroTask(std::bind(&CoroScheduler::gc_loop, this));
}

CoroScheduler::~CoroScheduler() {
  expired_coros_.clear();

  current_.reset();
  main_coro_.reset();
  gc_coro_.reset();;
}

RefCoroutine CoroScheduler::CurrentCoro() {
  return current_;
}

void CoroScheduler::YieldCurrent() {
  CHECK(main_coro_ != current_);
  Transfer(main_coro_);
}

intptr_t CoroScheduler::CurrentCoroId() {
  CHECK(current_);
  return current_->Identifier();
}

bool CoroScheduler::ResumeCoroutine(RefCoroutine& coro) {
  if (coro->resume_func_) {
    coro->resume_func_();
    return true;
  }
  if (InRootCoroutine()) {
    return Transfer(coro);
  }
  return false;
}

bool CoroScheduler::Transfer(RefCoroutine& next) {
  CHECK(current_ != next);

  next->SetCoroState(CoroState::kRunning);
  //Hack: this caused by GcCorotine
  if (current_->Status() != CoroState::kDone) {
    current_->SetCoroState(CoroState::kPaused);
  }

  coro_context* to = &next->coro_ctx_;
  coro_context* from = &current_->coro_ctx_;

  current_ = next;

  coro_transfer(from, to);
  return true;
}

bool CoroScheduler::InRootCoroutine() {
  return (main_coro_.get() && main_coro_ == current_);
}

void CoroScheduler::gc_loop() {
  while(1) {
    CHECK(current_ == gc_coro_);
    while(expired_coros_.size()) {
      expired_coros_.back().reset();
      expired_coros_.pop_back();
    }
    YieldCurrent();
  }
}

}// end base
