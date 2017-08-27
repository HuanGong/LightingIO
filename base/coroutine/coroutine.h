#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include "libcoro/coro.h"
#include "coroutine_task.h"

namespace base {

class Coroutine : public coro_context {
public:
  Coroutine();
  Coroutine(int stack_size, bool meta = false);
  Coroutine(std::unique_ptr<CoroTask> t, int stack_sz = 0);
  ~Coroutine();

  Coroutine* GetCaller();
  void SetCaller(Coroutine* caller);
  void SetCoroTask(std::unique_ptr<CoroTask> t);

  void Yield();
  void Transfer(Coroutine* next);


  void RunCoroTask();
private:
  void InitCoroutine();

  Coroutine* caller_;
  int stack_size_;

  //bool scheduled_;
  bool meta_coro_;
  coro_stack stack_;

  std::unique_ptr<CoroTask> task_;
};

}//end base
#endif
