#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include <cinttypes>
#include <atomic>
#include "config.h"
#include <memory>
#include <functional>
#include <base_micro.h>
#include "libcoro/coro.h"
#include <closure/closure_task.h>

namespace base {
typedef std::function<void()> CoroClosure;

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

  std::string StateToString() const;
  CoroState Status() const {return state_;}
  bool IsPaused() const {return state_ == kPaused;}
  bool IsRunning() const {return state_ == kRunning;}
  uint64_t Identify() const {return identify_;}
private:
  Coroutine(CoroDelegate* d, bool main);

  void Reset();
  void SetTask(ClosurePtr&& task);
  void SelfHolder(RefCoroutine& self);
  void SetCoroState(CoroState st) {state_ = st;}
  void ReleaseSelfHolder() {self_holder_.reset();};
  std::weak_ptr<Coroutine> AsWeakPtr() {return shared_from_this();}

private:
  int32_t wc_;
  CoroDelegate* delegate_;

  CoroState state_;
  coro_stack stack_;
  ClosurePtr coro_task_;
  /* 此identify_ 只有在每次从暂停状态转换成running状态之后才会自增; 用于防止非正确的唤醒; eg:
   * coro running -> coro paused 之后有多个事件都可能唤醒他，其中一个已经唤醒过此coro，唤醒之后
   * 再次puased， 若此前另外一个事件唤醒coro， 将导致不正确的状态.*/
  uint64_t identify_;

  RefCoroutine self_holder_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
#endif
