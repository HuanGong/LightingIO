
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include "closure/closure_task.h"

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

  LOG(INFO) << __FUNCTION__ << " : " << __LINE__;

  RefCoroutine coro = std::make_shared<Coroutine>();
  coro->SetCoroTask(std::move(task));

  TlsCurrent()->OnNewCoroBorn(coro);

  TlsCurrent()->Transfer(coro);
}

void CoroScheduler::OnNewCoroBorn(RefCoroutine& coro) {
  intptr_t id = coro->Identifier();
  coroutines_.insert(std::make_pair(id, coro));
}

void CoroScheduler::GcCoroutine(Coroutine* die) {
  CHECK(current_.get() == die &&
        die->Status() == CoroState::kDone);

  intptr_t id = die->Identifier();
  coroutines_.erase(id);

  expired_coros_.push_back(current_);
  Transfer(gc_coro_);
}

CoroScheduler::CoroScheduler() {

  main_coro_ = std::make_shared<Coroutine>(true);
  current_ = main_coro_;

  gc_coro_ = std::make_shared<Coroutine>(1024);
  gc_coro_->SetCoroTask(base::NewCoroTask(std::bind(&CoroScheduler::gc_loop, this)));
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
    return;
  }
  Transfer(main_coro_);
}

intptr_t CoroScheduler::CurrentCoroId() {
  CHECK(current_);
  return current_->Identifier();
}

bool CoroScheduler::ResumeCoroutine(intptr_t identifier) {
  if (!InRootCoroutine()) {
    return false;
  }

  IdCoroMap::iterator iter = coroutines_.find(identifier);
  if (coroutines_.end() == iter) {
    return false;
  }
  return Transfer(iter->second);
}

bool CoroScheduler::Transfer(RefCoroutine& next) {
  if (next == current_) {
    return true;
  }

  next->SetCoroState(CoroState::kRunning);
  //Hack: this caused by GcCorotine
  if (current_->Status() != CoroState::kDone) {
    current_->SetCoroState(CoroState::kPaused);
  }

  Coroutine* to = next.get();
  Coroutine* from = current_.get();

  current_ = next;
  coro_transfer(from, to);
  return true;
}

inline bool CoroScheduler::InRootCoroutine() {
  return main_coro_ && (main_coro_->Status() == CoroState::kRunning);
}

void CoroScheduler::gc_loop() {
  while(1) {
    while(expired_coros_.size()) {
      expired_coros_.back().reset();
      expired_coros_.pop_back();
    }
    YieldCurrent();
  }
}

}// end base
