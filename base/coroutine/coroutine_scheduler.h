#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include "base_micro.h"
#include "coroutine/coroutine.h"
#include "message_loop/message_loop.h"

namespace base {

class CoroScheduler {
public:
  CoroScheduler(MessageLoop* loop);
  ~CoroScheduler();

  void ScheduleCoro(Coroutine* coro);

protected:
  void schedule_coro(Coroutine* coro);
private:

  //a root coro created by base::coroutine(0, true);
  Coroutine* tls_main_coro_;
  MessageLoop* schedule_loop_;
  DISALLOW_COPY_AND_ASSIGN(CoroScheduler);
};


}// end base
#endif
