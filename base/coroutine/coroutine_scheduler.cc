
#include "coroutine_scheduler.h"
#include "message_loop/closure_task.h"

namespace base {

static thread_local CoroScheduler* scheduler_=nullptr;

void delete_coro(void* arg) {

}
//static thread safe
CoroScheduler* CoroScheduler::TlsCurrent() {
  if (!scheduler_ && MessageLoop::Current()) {
    scheduler_ = new CoroScheduler(MessageLoop::Current());
  }
  return scheduler_;
}
//static thread safe
void CoroScheduler::TlsDestroy() {
  delete scheduler_;
  scheduler_ = nullptr;
}

//static
Coroutine* CoroScheduler::CreateAndSchedule(std::unique_ptr<CoroTask> task) {
  if (!TlsCurrent()) {
    return nullptr;
  }
  LOG(INFO) << __FUNCTION__ << " : " << __LINE__;
  Coroutine* coro = new base::Coroutine();

  coro->SetCoroTask(std::move(task));

  TlsCurrent()->ScheduleCoro(coro);

  return coro;
}

void CoroScheduler::DeleteLater(Coroutine* finished) {
  //avoid self delete
  if (finished == coro_deleter_) {
    return;
  }
  LOG(INFO) << __FUNCTION__;
  trash_can_.push_back(finished);
  ScheduleCoro(coro_deleter_);
}
CoroScheduler::CoroScheduler(MessageLoop* loop)
  : main_coro_(nullptr),
    schedule_loop_(loop) {
  main_coro_ = new Coroutine(0, true);

  coro_deleter_ = new Coroutine();
  coro_deleter_->SetCoroTask(
    NewCoroTask(std::bind(&CoroScheduler::GcCoroutine, this)));
}

CoroScheduler::~CoroScheduler() {
  main_coro_ = nullptr;
  schedule_loop_ = nullptr;
}

void CoroScheduler::ScheduleCoro(Coroutine* coro) {
  if (coro->Status() == CoroState::kScheduled) {
    LOG(INFO) << "this coroutine aready scheduled";
    return;
  }
  LOG(INFO) << __FUNCTION__;
  coro->SetCoroState(CoroState::kScheduled);
  schedule_loop_->PostTask(
    NewClosure(std::bind(&CoroScheduler::schedule_coro, this, coro)));
}

void CoroScheduler::schedule_coro(Coroutine* coro) {
  LOG(INFO) << __FUNCTION__ << " from main_coro to a coro";
  DCHECK(main_coro_ == Coroutine::Current());
  main_coro_->Transfer(coro);
  LOG(INFO) << __FUNCTION__ << " back to main_coro from scheduled coro";
}

inline bool CoroScheduler::InRootCoroutine() {
  return main_coro_ && (main_coro_->Status() == CoroState::kRunning);
}

void CoroScheduler::GcCoroutine() {
  DCHECK(coro_deleter_ == Coroutine::Current());
  while(true) {
    if (!trash_can_.size()) {
      continue;
    }
    for (auto* coro : trash_can_) {
      delete coro;
    }
    trash_can_.clear();
    LOG(INFO) << __FUNCTION__ << " : " << __LINE__;
    coro_deleter_->Yield();
  }
}

}// end base
