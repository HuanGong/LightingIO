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
//#include "base/queue/double_linked_ref_list.h"

namespace base {
typedef std::set<RefCoroutine> CoroSet;


class CoroRunner : public CoroDelegate {
public:
  static bool CanYield();
  static RefCoroutine CurrentCoro();
  static void YieldCurrent(int32_t wc = 1);

  /* New Scheduler with message loop */
  static void RunScheduledTasks(std::list<StlClosure>&& tasks);
  /* ============ end ==============================*/
protected:
  CoroRunner();
  ~CoroRunner();

  /* swich call stack from different coroutine*/
  bool Transfer(const RefCoroutine& next);
  /* reuse: insert to freelist or gc: delete for free memory; then switch to thread coroutine*/
  void GcCoroutine();
  /* release the coroutine memory */
  void ReleaseExpiredCoroutine();
  /* case 1: scheduled_task exsist, set task and run again
   * case 2: scheduled_task empty, recall to freelist or gc it */
  void RecallCoroutineIfNeeded() override;
  /* a callback function using for resume a kPaused coroutine */
  void ResumeCoroutine(const RefCoroutine& coro);
  /* judge wheather running in a main coroutine with thread*/
  bool InMainCoroutine() const { return (main_coro_ == current_);}

  void InvokeCoroutineTasks();

  RefCoroutine RetrieveCoroutine();
private:
  RefCoroutine current_;
  RefCoroutine main_coro_;
  RefCoroutine gc_coro_;

  StlClosure gc_task_;
  bool gc_task_scheduled_;
  MessageLoop* bind_loop_;

  std::list<StlClosure> coro_tasks_;
  std::vector<RefCoroutine> expired_coros_;

  /* 根据系统实际负载分配的每个线程最大的可复用coroutine数量*/
  size_t max_reuse_coroutines_;
  std::list<RefCoroutine> free_list_;

  DISALLOW_COPY_AND_ASSIGN(CoroRunner);
};


}// end base
#endif
