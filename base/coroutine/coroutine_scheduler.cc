
#include "coroutine_scheduler.h"
#include "message_loop/closure_task.h"

namespace base {

static thread_local CoroScheduler* scheduler_=nullptr;

//static thread safe
const CoroScheduler* CoroScheduler::TlsCurrent() {
  if (!scheduler_ && MessageLoop::Current()) {
    scheduler_ = new Coroutine(MessageLoop::Current());
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
  coro->SetCaller(Coroutine::Current());

  coro->SetCoroTask(std::move(task));

  TlsCurrent()->ScheduleCoro(coro);

  return coroutine;
}


CoroScheduler::CoroScheduler(MessageLoop* loop)
  : tls_main_coro_(nullptr),
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
    NewClosure(std::bind(&ScheduleCoro::schedule_coro, this, coro)));
}

void CoroScheduler::schedule_coro(Coroutine* coro) {
  DCHECK(meta_coro_ == Coroutine::Current())
  main_coro_->Transfer(coro);
}

}// end base
