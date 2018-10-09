#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include <map>
#include <set>
#include <vector>
#include <cinttypes>
#include <unordered_map>
#include <unordered_set>
#include "base_micro.h"
#include <base/closure/closure_task.h>
#include "coroutine/coroutine.h"
#include <base/message_loop/message_loop.h>
//#include "base/queue/double_linked_ref_list.h"

namespace base {

class CoroRunner : public CoroDelegate {
public:
  typedef struct __go {
    __go(const char* func, const char* file, int line)
      : location_(func, file, line) {
    }

    template <typename Functor>
    inline void operator-(Functor arg) {
      CoroRunner& runner = Runner();
      runner.coro_tasks_.push_back(std::move(CreateClosure(location_, arg)));
      if (!runner.invoke_coro_shceduled_ && target_loop_) {
        target_loop_->PostTask(NewClosure(std::bind(&CoroRunner::InvokeCoroutineTasks, &runner)));
      }
    }
    inline __go& operator-(base::MessageLoop* loop) {
      target_loop_ = loop;
      return *this;
    }
    template <typename Functor>
    inline void operator<<(Functor closure_fn) {
      target_loop_->PostTask(NewClosure([&]() {
        CoroRunner& runner = Runner();
        runner.coro_tasks_.push_back(std::move(CreateClosure(location_, closure_fn)));
        if (!runner.invoke_coro_shceduled_ && target_loop_) {
          target_loop_->PostTask(NewClosure(std::bind(&CoroRunner::InvokeCoroutineTasks, &runner)));
        }
      }));
    }
    Location location_;
    base::MessageLoop* target_loop_ = MessageLoop::Current();
  }__go;

public:
  static bool CanYield();
  static CoroRunner& Runner();
  /* give up cpu; main coro will automatic resume; other coro should resume by manager*/
  static void YieldCurrent(int32_t wc = 1);
  static StlClosure CurrentCoroResumeCtx();
protected:
  CoroRunner();
  ~CoroRunner();

  /* swich call stack from different coroutine*/
  void TransferTo(Coroutine *next);
  /* reuse: insert to freelist or gc: delete for free memory; then switch to thread coroutine*/
  void GcCoroutine();
  /* release the coroutine memory */
  void ReleaseExpiredCoroutine();
  /* case 1: scheduled_task exsist, set task and run again
   * case 2: scheduled_task empty, recall to freelist or gc it */
  void RecallCoroutineIfNeeded() override;
  /* a callback function using for resume a kPaused coroutine */
  void ResumeCoroutine(std::weak_ptr<Coroutine> coro, uint64_t id);
  /* judge wheather running in a main coroutine with thread*/
  bool InMainCoroutine() const { return (main_coro_ == current_);}

  void InvokeCoroutineTasks();

  Coroutine* RetrieveCoroutine();
private:
  Coroutine* current_;
  Coroutine* main_coro_;

  StlClosure gc_task_;
  bool gc_task_scheduled_;
  MessageLoop* bind_loop_;

  bool invoke_coro_shceduled_;
  std::list<ClosurePtr> coro_tasks_;
  std::vector<Coroutine*> expired_coros_;

  /* 根据系统实际负载分配的每个线程最大的可复用coroutine数量*/
  size_t max_reuse_coroutines_;
  std::list<Coroutine*> free_list_;

  DISALLOW_COPY_AND_ASSIGN(CoroRunner);
};

#define go base::CoroRunner::__go(__FUNCTION__, __FILE__, __LINE__)-

}// end base
#endif
