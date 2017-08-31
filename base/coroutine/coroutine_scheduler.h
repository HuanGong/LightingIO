#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include "base_micro.h"
#include "coroutine/coroutine.h"
#include "message_loop/message_loop.h"

namespace base {

class CoroScheduler {
public:
  static void TlsDestroy();
  static CoroScheduler* TlsCurrent();

  static Coroutine* CreateAndSchedule(std::unique_ptr<CoroTask> task);

  bool InRootCoroutine();
  void ScheduleCoro(Coroutine* coro);
  void DeleteLater(Coroutine* finished);
protected:
  CoroScheduler(MessageLoop* loop);
  ~CoroScheduler();

  void GcCoroutine();
  void schedule_coro(Coroutine* coro);
private:
  //a root coro created by base::coroutine(0, true);
  Coroutine* main_coro_;
  Coroutine* coro_deleter_;
  MessageLoop* schedule_loop_;

  std::vector<Coroutine*> trash_can_;
  DISALLOW_COPY_AND_ASSIGN(CoroScheduler);
};


}// end base
#endif
