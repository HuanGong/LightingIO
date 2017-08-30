
#include "coroutine_scheduler.h"
#include "message_loop/closure_task.h"

namespace base {

static thread_local CoroScheduler* scheduler_=nullptr;

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
  Coroutine* coro = new base::Coroutine();
  //important, this ensure back to the
  //parent coro after finishing work
  //coro->SetCaller(Coroutine::Current());

  coro->SetCoroTask(std::move(task));

  TlsCurrent()->ScheduleCoro(coro);

  return coro;
}


CoroScheduler::CoroScheduler(MessageLoop* loop)
  : main_coro_(nullptr),
    schedule_loop_(loop) {
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
  schedule_loop_->PostTask(
    NewClosure(std::bind(&CoroScheduler::schedule_coro, this, coro)));
}

void CoroScheduler::schedule_coro(Coroutine* coro) {
  DCHECK(main_coro_ == Coroutine::Current());
  main_coro_->Transfer(coro);
}

}// end base
