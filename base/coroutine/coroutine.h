#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include "libcoro/coro.h"

namespace base {

class Coroutine : public coro_context {
public:
  Coroutine(int stack_size, bool meta = false);
  ~Coroutine();

  void SetCaller(Coroutine* caller);
  Coroutine* GetCaller();

  void Yield();
  void Transfer(Coroutine* next);
private:
  Coroutine* caller_;
  int stack_size_;

  bool scheduled_;
  coro_stack coro_stack_;
  bool meta_coro_;
};

}//end base
#endif
