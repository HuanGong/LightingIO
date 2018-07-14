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

  static void RunAsCoroInLoop(base::MessageLoop* target_loop, StlClosure& t);
  static void CreateAndSchedule(CoroClosure& task);

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
  DISALLOW_COPY_AND_ASSIGN(CoroScheduler);
};


}// end base
#endif
