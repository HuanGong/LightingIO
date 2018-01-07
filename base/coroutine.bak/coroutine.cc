
#include "coroutine.h"
#include <vector>
#include <iostream>
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include <atomic>

namespace base {
static thread_local Coroutine* tls_current_ = nullptr;
std::atomic<long> count;

//static
void Coroutine::run_coroutine(void* arg) {
  CHECK(arg);
  Coroutine* coroutine = static_cast<Coroutine*>(arg);

  LOG(INFO) << __FUNCTION__ << " RUN" << std::endl;
  coroutine->RunCoroTask();

  coroutine->current_state_ = CoroState::KFinished;

  Coroutine* superior = coroutine->GetSuperior();
  Coroutine* call_stack_prarent = coroutine->GetCaller();

  CoroScheduler::TlsCurrent()->DeleteLater(coroutine);
  if (superior) {
    superior->current_state_ = CoroState::kRunning;
    tls_current_ = superior;
    coro_transfer(coroutine, superior);
  } else if (call_stack_prarent) {
    tls_current_ = call_stack_prarent;
    call_stack_prarent->current_state_ = CoroState::kRunning;

    coro_transfer(coroutine, call_stack_prarent);
  }
  LOG(ERROR) << __FUNCTION__ << " SHOULD NOT REACHED!";
}

//static
Coroutine* Coroutine::Current() {
  return tls_current_;
}

Coroutine::Coroutine()
  : superior_(nullptr),
    caller_(nullptr),
    stack_size_(0),
    meta_coro_(false) {

  InitCoroutine();
}

Coroutine::Coroutine(std::unique_ptr<CoroTask> t, int stack_sz)
  : superior_(nullptr),
    caller_(nullptr),
    stack_size_(0),
    meta_coro_(false),
    task_(std::move(t)) {

  InitCoroutine();
}

Coroutine::Coroutine(int stack_size, bool meta_coro)
  : superior_(nullptr),
    caller_(nullptr),
    stack_size_(stack_size),
    meta_coro_(meta_coro) {

  InitCoroutine();
}

Coroutine::~Coroutine() {
  coro_stack_free(&stack_);
  count--;
  LOG(INFO) << "coroutine count:" << count;
}

void Coroutine::InitCoroutine() {
  if (meta_coro_) {
    tls_current_ = this;
    current_state_ = CoroState::kRunning;
    coro_create(this, NULL, NULL, NULL, 0);
  } else {
    count++;
    LOG(INFO) << "coroutine count:" << count;
    coro_stack_alloc(&stack_, stack_size_);
    coro_create(this, // coro_context
                Coroutine::run_coroutine, //func
                this, //arg
                stack_.sptr,
                stack_.ssze);
    current_state_ = CoroState::kJustReady;
  }
  VLOG(4) << __FUNCTION__
             << " MainCoro:" << meta_coro_
             << " stack_size:" << stack_size_;
}

Coroutine* Coroutine::GetCaller() {
  return caller_;
}

void Coroutine::SetSuperior(Coroutine* su) {
  superior_ = su;
}

void Coroutine::SetCaller(Coroutine* caller) {
  caller_ = caller;
}

void Coroutine::Yield() {
  Coroutine* caller = GetCaller();
  if (!caller) {
    return;
  }
  tls_current_ = caller;
  current_state_ = CoroState::kPaused;
  caller->current_state_ = CoroState::kRunning;

  coro_transfer(this, caller);
}

void Coroutine::Transfer(Coroutine* next) {
  //avoid a nother running coro: call PausedCoro->Transfer(Another Coro)
  if (next == this || current_state_ != CoroState::kRunning) {
    LOG(INFO) << __FUNCTION__ << ":" << __LINE__ << " Transfer Failed";
    return;
  }
  next->SetCaller(this);

  tls_current_ = next;
  current_state_ = CoroState::kPaused;
  next->current_state_ = CoroState::kRunning;
  coro_transfer(this, next);
}

void Coroutine::RunCoroTask() {
  if (!task_.get()) {
    LOG(INFO) << "this coro not Bind To a Task";
    return;
  }
  task_->RunCoro();
  task_.reset();
}

void Coroutine::SetCoroTask(std::unique_ptr<CoroTask> t) {
  if (task_.get()) {
    LOG(ERROR) << "A Task Aready Set, Coroutine Can't Assign Task Twice";
    return;
  }
  task_ = std::move(t);
}

}//end base
