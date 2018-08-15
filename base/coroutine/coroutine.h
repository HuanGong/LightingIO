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
  kRunning = 1,
  kPaused = 2,
  kDone = 3
};
class Coroutine;
typedef std::shared_ptr<Coroutine> RefCoroutine;
typedef std::function<void(Coroutine*)> CoroCallback;

int64_t SystemCoroutineCount();

class Coroutine : public coro_context {
public:
  friend class CoroScheduler;
  friend void coro_main(void* coro);

  static std::shared_ptr<Coroutine> Create(bool main = false);
  ~Coroutine();

  bool Resume();
  std::string StateToString() const;
  const CoroState Status() const {return current_state_;}
  uint64_t TaskIdentify() const {return task_identify_;}

private:
  Coroutine(bool main);
  void Reset();
  void SetTask(CoroClosure task);
  void SetCoroState(CoroState st) {current_state_ = st;}
  void SetRecallCallback(CoroCallback cb) {recall_callback_ = cb;}
  void SelfHolder(RefCoroutine& self);
  void ReleaseSelfHolder(){self_holder_.reset();};

  coro_stack stack_;
  CoroState current_state_;

  CoroClosure coro_task_;
  uint64_t task_identify_;

  StlClosure resume_func_;
  RefCoroutine self_holder_;
  CoroCallback start_callback_;
  CoroCallback recall_callback_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
#endif
