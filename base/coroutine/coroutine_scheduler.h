#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include <map>
#include <vector>
#include <cinttypes>
#include <unordered_map>
#include "base_micro.h"
#include "coroutine/coroutine.h"

namespace base {

class CoroScheduler {
public:
  typedef std::shared_ptr<Coroutine> RefCoroutine;
  typedef std::map<intptr_t, RefCoroutine> IdCoroMap;

  static CoroScheduler* TlsCurrent();

  static void CreateAndSchedule(std::unique_ptr<CoroTask> task);

  intptr_t CurrentCoroId();
  void YieldCurrent();
  bool InRootCoroutine();

  bool ResumeCoroutine(intptr_t identifier);
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

  IdCoroMap coroutines_;

  std::vector<RefCoroutine> expired_coros_;
  DISALLOW_COPY_AND_ASSIGN(CoroScheduler);
};


}// end base
#endif
