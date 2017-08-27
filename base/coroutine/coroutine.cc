
#include "coroutine.h"
#include <vector>
#include <iostream>
#include "glog/logging.h"

namespace base {
static std::vector<Coroutine*> finished_tasks_;

static void run_coroutine(void* arg) {
  CHECK(arg);
  LOG(INFO) << __FUNCTION__ << " RUN" << std::endl;
  for (auto& v : finished_tasks_) {
    delete v;
  }
  finished_tasks_.clear();
  Coroutine* coroutine = static_cast<Coroutine*>(arg);
  //IMPORTANT!! here save the call stack parent, when call
  //RunCoroTask May Yield Again and Again, it my change the
  //Caller, so here need save it
  Coroutine* call_stack_prarent = coroutine->GetCaller();

  // do coro work here
  coroutine->RunCoroTask();

  //this coro is finished, now we need back to parent call stack
  //and nerver came back again, so here can't be Transfer，bz Transfer
  //will set the caller

  finished_tasks_.push_back(coroutine);
  if (call_stack_prarent) {
    // set to parent to corotine contex
    // CorotineContext::SetCurrent(Call_stack_parent)
    coro_transfer(coroutine, call_stack_prarent);
  }
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
    coro_create(this, NULL, NULL, NULL, 0);
  } else {
    coro_stack_alloc(&stack_, stack_size_);
    coro_create(this, // coro_context
                run_coroutine, //func
                this, //arg
                stack_.sptr,
                stack_.ssze);
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
  CHECK(GetCaller());
  // CorotineContext::SetCurrent(Caller)
  coro_transfer(this, GetCaller());
}

void Coroutine::Transfer(Coroutine* next) {
  CHECK(next != this); //avoid infinite loop

  next->SetCaller(this);
  // CorotineContext::SetCurrent(next)
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
  }
  task_ = std::move(t);
}

}//end base
