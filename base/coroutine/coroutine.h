#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include <cinttypes>
#include "config.h"
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

class Coroutine : public coro_context {
public:
  typedef std::shared_ptr<Coroutine> RefCoroutine;
  typedef std::function<void(Coroutine*)> CoroCallback;

  static std::shared_ptr<Coroutine> Create(bool main = false);
  static int64_t SystemCoroutineCount();
  ~Coroutine();

  void SetCoroTask(CoroClosure task);
  const CoroState Status() const {return current_state_;}
protected:
  friend class CoroScheduler;
  Coroutine(bool main);
  static void run_coroutine(void* arg);

  void Reset();
  void RunCoroutineTask();
  void SetCoroState(CoroState st) {current_state_ = st;}

  coro_stack stack_;

  CoroState current_state_;
  CoroClosure coro_task_;

  StlClosure resume_func_;

  CoroCallback start_callback_;
  CoroCallback recall_callback_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
#endif
