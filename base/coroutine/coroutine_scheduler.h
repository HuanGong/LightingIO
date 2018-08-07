#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include <map>
#include <set>
#include <vector>
#include <cinttypes>
#include <unordered_map>
#include "base_micro.h"
#include <base/closure/closure_task.h>
#include "coroutine/coroutine.h"

#include <base/message_loop/message_loop.h>

namespace base {

typedef std::shared_ptr<Coroutine> RefCoroutine;
class CoroScheduler {
public:
  //typedef std::map<intptr_t, RefCoroutine> IdCoroMap;
  typedef std::set<RefCoroutine> CoroSet;

  static CoroScheduler* TlsCurrent();

  static void ScheduleCoroutineInLoop(base::MessageLoop* target_loop, StlClosure& t);
  static void CreateAndTransfer(CoroClosure& task);

  intptr_t CurrentCoroId();
  RefCoroutine CurrentCoro();
  void YieldCurrent();
  bool InRootCoroutine();

  bool ResumeCoroutine(RefCoroutine& coro);
  void GcCoroutine(Coroutine* finished);
protected:
  CoroScheduler();
  ~CoroScheduler();

  void OnNewCoroBorn(RefCoroutine& coro);
  bool Transfer(RefCoroutine& next);
private:
  void gc_loop();

  RefCoroutine current_;
  RefCoroutine main_coro_;
  RefCoroutine gc_coro_;

  //IdCoroMap coroutines_;
  CoroSet coroutines_;

  std::vector<RefCoroutine> expired_coros_;

  /* 根据系统实际负载分配的每个线程最大的可复用coroutine数量*/
  size_t max_reuse_coroutines_;
  std::list<RefCoroutine> free_list_;
  DISALLOW_COPY_AND_ASSIGN(CoroScheduler);
};


}// end base
#endif
