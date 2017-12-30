
#include "coroutine.h"
#include <vector>
#include <iostream>
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include <atomic>

namespace base {

#define COROSTACKSIZE 16384

std::atomic<long> count;

//static
void Coroutine::run_coroutine(void* arg) {
  CHECK(arg);
  LOG(INFO) << __FUNCTION__ << " RUN" << std::endl;

  Coroutine* coroutine = static_cast<Coroutine*>(arg);

  coroutine->current_state_ = CoroState::kRunning;

  coroutine->RunCoroTask();

  coroutine->current_state_ = CoroState::kDone;

  CoroScheduler::TlsCurrent()->GcCoroutine(coroutine);
}

Coroutine::Coroutine()
  : stack_size_(COROSTACKSIZE),
    meta_coro_(false) {

  InitCoroutine();
}

Coroutine::Coroutine(int stack_sz)
  : stack_size_(stack_sz),
    meta_coro_(false) {

  InitCoroutine();
}

Coroutine::Coroutine(bool meta_coro)
  : stack_size_(0),
    meta_coro_(meta_coro) {
  stack_size_ =  meta_coro_ ? 0 : COROSTACKSIZE;
  InitCoroutine();
}

Coroutine::~Coroutine() {
  count--;
  LOG(INFO) << "coroutine gone! count:" << count << "st:" << current_state_;
  CHECK(current_state_ == kDone);
  coro_stack_free(&stack_);
}

void Coroutine::InitCoroutine() {
  if (meta_coro_) {
    current_state_ = CoroState::kRunning;
    coro_create(this, NULL, NULL, NULL, 0);
  } else {
    if (0 == stack_size_) stack_size_ = COROSTACKSIZE;
    coro_stack_alloc(&stack_, stack_size_);
    coro_create(this, // coro_context
                Coroutine::run_coroutine, //func
                this, //arg
                stack_.sptr,
                stack_.ssze);
    current_state_ = CoroState::kInitialized;
  }

  count++;
  LOG(INFO) << "coroutine born count:" << count;
}

void Coroutine::RunCoroTask() {
  if (!task_.get()) {
    LOG(INFO) << "This coro not Bind To a Task";
    return;
  }
  task_->RunCoro();
  task_.reset();
}

void Coroutine::SetCoroTask(std::unique_ptr<CoroTask> t) {
  if (task_.get()) {
    LOG(ERROR) << "Aready Set, Can't Assign Task Twice";
    return;
  }
  task_ = std::move(t);
}

intptr_t Coroutine::Identifier() const {
  return (intptr_t)this;
}

}//end base
