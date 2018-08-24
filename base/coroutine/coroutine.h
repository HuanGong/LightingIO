#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include <cinttypes>
#include <atomic>
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
class CoroDelegate {
public:
  virtual void RecallCoroutineIfNeeded() = 0;
};

class Coroutine : public coro_context,
                  public std::enable_shared_from_this<Coroutine> {
public:
  friend class CoroRunner;
  friend void coro_main(void* coro);

  static std::shared_ptr<Coroutine> Create(CoroDelegate* d, bool main = false);
  ~Coroutine();

  bool Resume(uint64_t id);
  std::string StateToString() const;
  const CoroState Status() const {return state_.load();}
  uint64_t TaskIdentify() const {return task_identify_;}
private:
  Coroutine(CoroDelegate* d, bool main);
  void Reset();
  void SetTask(CoroClosure task);
  void SetCoroState(CoroState st) {state_ = st;}
  void SelfHolder(RefCoroutine& self);
  void ReleaseSelfHolder(){self_holder_.reset();};

  int32_t wc_;
  CoroDelegate* delegate_;
  coro_stack stack_;
  std::atomic<CoroState> state_;
  CoroClosure coro_task_;
  uint64_t task_identify_;
  StlClosure resume_func_;
  RefCoroutine self_holder_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
#endif
