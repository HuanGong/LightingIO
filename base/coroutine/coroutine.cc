
#include "coroutine.h"
#include <iostream>

#include "glog/logging.h"

namespace base {

static void RunCoroTask(void* arg) {
  LOG(INFO) << __FUNCTION__ << " RUN" << std::endl;
  Coroutine* coroutine = static_cast<Coroutine*>(arg);
  coroutine->Yield();
}

Coroutine::Coroutine(int stack_size, bool meta)
  : caller_(nullptr),
    stack_size_(stack_size) {
  coro_stack_alloc(&coro_stack_, stack_size_);

  if (meta) {
    coro_create(this, NULL, NULL, NULL, 0);
  } else {
    coro_create(this,
                RunCoroTask,
                this,
                coro_stack_.sptr,
                coro_stack_.ssze);
  }
  //LOG(INFO) << __FUNCTION__ << " RUN";
  std::cout << __FUNCTION__ << "line " << __LINE__ << std::endl;
}

Coroutine::~Coroutine() {
  if (scheduled_ && !meta_coro_) {
    Yield();
  }
  coro_stack_free(&coro_stack_);
  std::cout << __FUNCTION__ << "line " << __LINE__ << std::endl;
  LOG(INFO) << __FUNCTION__ << " RUN";
}

Coroutine* Coroutine::GetCaller() {
  return caller_;
}
void Coroutine::SetCaller(Coroutine* caller) {
  caller_ = caller;
}

void Coroutine::Yield() {
  //CHECK(GetCaller());
  coro_transfer(this, GetCaller());
}
void Coroutine::Transfer(Coroutine* next) {
  //CHECK(GetCaller());
  next->SetCaller(this);
  coro_transfer(this, next);
}

}//end base
