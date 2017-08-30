#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include "libcoro/coro.h"
#include "coroutine_task.h"

namespace base {

enum CoroState {
  kJustReady = 0,
  kScheduled = 1,
  kRunning = 2,
  kPaused = 3,
  KFinished = 3
};

class Coroutine : public coro_context {
public:
  Coroutine();
  Coroutine(int stack_size, bool meta = false);
  Coroutine(std::unique_ptr<CoroTask> t, int stack_sz = 0);
  ~Coroutine();

  CoroState Status() {return current_state_; }
  Coroutine* GetSuperior() {return superior_;}
  Coroutine* GetCaller();
  void SetSuperior(Coroutine* su);
  void SetCoroTask(std::unique_ptr<CoroTask> t);

  void Yield();
  void Transfer(Coroutine* next);

  static Coroutine* Current();
private:
  void InitCoroutine();
  void RunCoroTask();
  void SetCaller(Coroutine* caller);

  static void run_coroutine(void* arg);

private:
  Coroutine* superior_ ;
  Coroutine* caller_;
  int stack_size_;

  //bool scheduled_;
  bool meta_coro_;
  coro_stack stack_;
  CoroState current_state_;
  std::unique_ptr<CoroTask> task_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
#endif
