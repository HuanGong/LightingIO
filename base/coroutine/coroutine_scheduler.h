#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include "base_micro.h"
#include "coroutine/coroutine.h"
#include "message_loop/message_loop.h"

namespace base {

class CoroScheduler {
public:
  static const CoroScheduler* Current();
  static void TlsDestroy();

  static bool CreateAndSchedule(std::unique_ptr<CoroTask> task);

  void ScheduleCoro(Coroutine* coro);
protected:
  CoroScheduler(MessageLoop* loop);
  ~CoroScheduler();

  void schedule_coro(Coroutine* coro);
private:
  //a root coro created by base::coroutine(0, true);
  Coroutine* main_coro_;
  MessageLoop* schedule_loop_;
  DISALLOW_COPY_AND_ASSIGN(CoroScheduler);
};


}// end base
#endif
