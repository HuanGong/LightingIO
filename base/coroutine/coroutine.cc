
#include "coroutine.h"
#include <vector>
#include <iostream>
#include "glog/logging.h"

namespace base {
static thread_local Coroutine* tls_current_ = nullptr;

//static
void Coroutine::run_coroutine(void* arg) {
  CHECK(arg);
  Coroutine* coroutine = static_cast<Coroutine*>(arg);

  //IMPORTANT!! here save the call stack parent, when call
  //RunCoroTask May Yield Again and Again, it my change the
  //Caller, so here need save it
  Coroutine* call_stack_prarent = coroutine->GetCaller();

  LOG(INFO) << __FUNCTION__ << " RUN" << std::endl;
  // do coro work here
  coroutine->RunCoroTask();

  //this coro is finished, now we need back to parent call stack
  //and nerver came back again, so here can't use coro->Transfer，
  //bz func Transfer will set the caller
  if (call_stack_prarent) {
    // CorotineContext::SetCurrent(Call_stack_parent)
    tls_current_ = call_stack_prarent;
    coroutine->current_state_ = CoroState::KFinished;
    call_stack_prarent->current_state_ = CoroState::kRunning;
    coro_transfer(coroutine, call_stack_prarent);
  }
}

//static
Coroutine* Coroutine::Current() {
  return tls_current_;
}

Coroutine::Coroutine()
  : caller_(nullptr),
    stack_size_(0),
    meta_coro_(false) {

  InitCoroutine();
}

Coroutine::Coroutine(std::unique_ptr<CoroTask> t, int stack_sz)
  : caller_(nullptr),
    stack_size_(0),
    meta_coro_(false),
    task_(std::move(t)) {

  InitCoroutine();
}

Coroutine::Coroutine(int stack_size, bool meta_coro)
  : caller_(nullptr),
    stack_size_(stack_size),
    meta_coro_(meta_coro) {

  InitCoroutine();
}

Coroutine::~Coroutine() {
  coro_stack_free(&stack_);
}

void Coroutine::InitCoroutine() {
  if (meta_coro_) {
    tls_current_ = this;
    current_state_ = CoroState::kRunning;
    coro_create(this, NULL, NULL, NULL, 0);
  } else {
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
void Coroutine::SetCaller(Coroutine* caller) {
  caller_ = caller;
}

void Coroutine::Yield() {
  Coroutine* caller = GetCaller();
  CHECK(caller);

  tls_current_ = caller;
  current_state_ = CoroState::kPaused;
  caller->current_state_ = CoroState::kRunning;

  coro_transfer(this, caller);
}

void Coroutine::Transfer(Coroutine* next) {
  //avoid a nother running coro: call PausedCoro->Transfer(Another Coro)
  if (next != this || current_state_ != CoroState::kRunning) {
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
