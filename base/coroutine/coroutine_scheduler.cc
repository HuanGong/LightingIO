
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include "closure/closure_task.h"
#include "memory/lazy_instance.h"

namespace base {

//static thread safe
CoroScheduler* CoroScheduler::TlsCurrent() {
  //static thread_local base::LazyInstance<CoroScheduler> scheduler = LAZY_INSTANCE_INIT;
  static thread_local CoroScheduler scheduler;
  return &scheduler;
}

//static
void CoroScheduler::RunAsCoroInLoop(base::MessageLoop* target_loop, StlClosure& t) {
  CHECK(target_loop && t);

  StlClosure coro_functor = std::bind(&CoroScheduler::CreateAndSchedule, t);

  target_loop->PostTask(base::NewClosure(coro_functor));
}

//static
void CoroScheduler::CreateAndSchedule(CoroClosure& task) {

  CHECK(TlsCurrent()->InRootCoroutine());

  RefCoroutine coro = std::make_shared<Coroutine>(0, false);

  coro->SetCoroTask(std::move(task));

  TlsCurrent()->OnNewCoroBorn(coro);

  TlsCurrent()->Transfer(coro);
}

void CoroScheduler::OnNewCoroBorn(RefCoroutine& coro) {
  coroutines_.insert(coro);
}

void CoroScheduler::GcCoroutine(Coroutine* die) {
  coroutines_.erase(current_);

  expired_coros_.push_back(current_);

  expired_coros_.size() > 100 ? Transfer(gc_coro_) : Transfer(main_coro_);
}

CoroScheduler::CoroScheduler() {
  main_coro_.reset(new Coroutine(0, true));
  current_ = main_coro_;

  gc_coro_.reset(new Coroutine(0, false));//std::make_shared<Coroutine>(true);
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
  CHECK(InRootCoroutine());

  if (!InRootCoroutine()) {
    return false;
  }
  return Transfer(coro);
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
