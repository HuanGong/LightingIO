#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include <cinttypes>
#include "libcoro/coro.h"
#include "coroutine_task.h"
#include "closure/closure_task.h"

namespace base {

enum CoroState {
  kInitialized = 0,
  kScheduled = 1,
  kRunning = 2,
  kPaused = 3,
  kDone = 4
};

// only can allocate in stack
class Coroutine : public coro_context {
public:
  intptr_t Identifier() const;
  const CoroState Status() const {return current_state_;}

  Coroutine(int stack_size, bool main);
  ~Coroutine();
private:
  friend class CoroScheduler;
  static void run_coroutine(void* arg);

  void Reset();
  void SetIdentifier(uint64_t id);
  void SetCoroTask(CoroClosure task);
  inline void SetCoroState(CoroState st) {current_state_ = st;}

  const bool meta_coro_;

  int stack_size_;
  coro_stack stack_;
  coro_context coro_ctx_;

  CoroState current_state_;

  CoroClosure coro_task_;

  StlClourse resume_func_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
#endif
