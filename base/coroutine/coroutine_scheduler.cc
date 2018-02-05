
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include "closure/closure_task.h"
#include <iostream>
#include <base/event_loop/msg_event_loop.h>

namespace base {

static thread_local CoroScheduler* scheduler_=nullptr;

//static thread safe
CoroScheduler* CoroScheduler::TlsCurrent() {
  if (!scheduler_) {
    scheduler_ = new CoroScheduler();
  }
  return scheduler_;
}

//static
void CoroScheduler::CreateAndSchedule(std::unique_ptr<CoroTask> task) {

  CHECK(TlsCurrent()->InRootCoroutine());

  //LOG(ERROR) << __FUNCTION__ << " : " << __LINE__ << " @ Loop " << base::MessageLoop2::Current()->LoopName();

  RefCoroutine coro = std::make_shared<Coroutine>(0, false);
  if (!coro.get()) {
    LOG(ERROR) << "Bad Coro Seem Failed" << " @ Loop " << base::MessageLoop2::Current()->LoopName();
    return;
  }
  coro->SetCoroTask(std::move(task));

  //LOG(ERROR) << __FUNCTION__ << " from Here:" << TlsCurrent()->CurrentCoro().get() << " to:" << coro.get();
  TlsCurrent()->OnNewCoroBorn(coro);

  TlsCurrent()->Transfer(coro);
}

void CoroScheduler::OnNewCoroBorn(RefCoroutine& coro) {
  //intptr_t id = coro->Identifier();
  coroutines_.insert(coro);
}

void CoroScheduler::GcCoroutine(Coroutine* die) {
  if (current_.get() != die) {
    LOG(ERROR) << " main_coro_:" << main_coro_.get();
    LOG(ERROR) << " gc_coro_:" << gc_coro_.get();
    LOG(ERROR) << " current_:" << current_.get();
  }
  CHECK(current_.get() == die);
  CHECK(die->Status() == CoroState::kDone);

  //intptr_t id = die->Identifier();
  coroutines_.erase(current_);

  expired_coros_.push_back(current_);
  Transfer(gc_coro_);
}

CoroScheduler::CoroScheduler() {
  //LOG(ERROR) << " constructor CoroScheduler" << " @ loop" << base::MessageLoop2::Current()->LoopName();

  main_coro_.reset(new Coroutine(0, true));
  current_ = main_coro_;

  gc_coro_.reset(new Coroutine(0, false));//std::make_shared<Coroutine>(true);
  gc_coro_->SetCoroTask(base::NewCoroTask(std::bind(&CoroScheduler::gc_loop, this)));
  //LOG(ERROR) << " constructor Leave, current set to main_coro_" << " @ loop" << base::MessageLoop2::Current()->LoopName();
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
  if (main_coro_ == current_) {
    LOG(ERROR) << "Can't Yield root coro";
  }
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

  LOG_IF(ERROR, next == main_coro_) << " current set to main_coro_";
  current_ = next;

  coro_transfer(from, to);
  return true;
}

bool CoroScheduler::InRootCoroutine() {
  return (main_coro_.get() && main_coro_ == current_);
  //(main_coro_->Status() == CoroState::kRunning);
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
