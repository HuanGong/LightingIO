#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include <map>
#include <set>
#include <vector>
#include <cinttypes>
#include <base/base_micro.h>
#include <base/message_loop/message_loop.h>

#ifdef USE_LIBACO_CORO_IMPL
#include "aco_impl.h"
#else
#include "coro_impl.h"
#endif

namespace base {

typedef std::function<void()> CoroResumer;

/*
 * same thing like go language Goroutine CSP model,
 * But with some aspect diffirence
 *
 * G: CoroTask
 * M: Coroutine
 * P: CoroRunner <-bind-> MessageLoop
 *
 * __go is a utility for schedule a CoroTask(G) Bind To
 * Current(can also bind to a specific native thread) CoroRunner(P)
 *
 * when invoke the G(CoroTask), P(CoroRunner) will choose a suitable
 * M to excute the task
 * */

class CoroRunner : public PersistRunner{
public:
  typedef struct _go {
    _go(const char* func, const char* file, int line)
      : location_(func, file, line),
        target_loop_(_go::loop()) {
    }

    /* specific a loop for this task*/
    inline _go& operator-(base::MessageLoop* loop) {
      target_loop_ = loop;
      return *this;
    }

    /* schedule a corotine task to stealing_queue, and weakup a
     * target loop to run this task, but this task can be invoke
     * by other loop, if you want task invoke in specific loop
     * use `co_go &loop << Functor`
     * */
    template <typename Functor>
    inline void operator-(Functor func) {
      CoroRunner::schedule_task(NewClosure(func));
      target_loop_->WeakUp();
    }

    // here must make sure all things wraper(copy) into closue,
    // becuase __go object will destruction before task closure run
    template <typename Functor>
    inline void operator<<(Functor closure_fn) {
    	auto func = [=](base::MessageLoop* loop, const Location& location) {
        CoroRunner& runner = Runner();
        runner.ScheduleTask(CreateClosure(location, closure_fn));
        loop->WeakUp();
    	};
      target_loop_->PostTask(FROM_HERE, func, target_loop_, location_);
    }

    inline static base::MessageLoop* loop() {
      base::MessageLoop* current = MessageLoop::Current();
      return  current ? current : CoroRunner::backgroup();
    }
    Location location_;
    base::MessageLoop* target_loop_ = nullptr;
  }_go;

public:
  friend class _go;

  static CoroRunner& Runner();

  /* here two ways registe loop as coro runner worker
   * 1. implicit call CoroRunner::Runner()
   * 2. call RegisteAsCoroWorker manually
   * */
  static void RegisteAsCoroWorker(base::MessageLoop*);

  void Yield();

  StlClosure Resumer();

  void Sleep(uint64_t ms);

  bool CanYieldHere() const {return !IsMain();}

  std::string RunnerInfo() const;
protected:
#ifdef USE_LIBACO_CORO_IMPL
  static void CoroutineEntry();
#else
  static void CoroutineEntry(void *coro);
#endif

  static ConcurrentTaskQueue stealing_queue;

  static base::MessageLoop* backgroup();

  static bool schedule_task(TaskBasePtr&& task);

  CoroRunner();
  ~CoroRunner();

  /* override from PersistRunner
   *
   * load(bind) task(cargo) to coroutine(car) and run(transfer)
   * */
  void Sched();

  void ScheduleTask(TaskBasePtr&& task);

  /* release the coroutine memory */
  void FreeOutdatedCoro();

  /* check whether still has pending task need to run
   * case 0: still has pending task need to run
   *  return true at once
   * case 1: no pending task need to run
   *    1.1: enough coroutine(task executor)
   *      return false; then corotuine will end its life time
   *    1.2: just paused this corotuine, and wait task to resume it
   *      return true
   */
  bool ContinueRun();

  /*return true when has more task to run*/
  bool HasMoreTask() const;

  /*retrieve task from inloop queue or public task pool*/
  bool GetTask(TaskBasePtr& task);

  /* from stash list got a coroutine or create new one*/
  Coroutine* RetrieveCoroutine();

  /* a callback function using for resume a kPaused coroutine */
  void Resume(WeakCoroutine& coro, uint64_t id);

  /* do resume a coroutine from main_coro*/
  void DoResume(WeakCoroutine& coro, uint64_t id);

  bool IsMain() const {return main_coro_ == current_;}

  /* switch call stack from different coroutine*/
  void SwapCurrentAndTransferTo(Coroutine *next);
private:
  Coroutine* current_;
  RefCoroutine main_;
  Coroutine* main_coro_;

  bool gc_task_scheduled_;
  MessageLoop* bind_loop_;

  std::list<TaskBasePtr> coro_tasks_;
  std::vector<Coroutine*> to_be_delete_;

  /*every thread max resuable coroutines*/
  size_t max_parking_count_;
  std::list<Coroutine*> stash_list_;
  DISALLOW_COPY_AND_ASSIGN(CoroRunner);
};

}// end base

//NOTE: co_yield,co_await,co_resume has bee reserved keywords for c++20,
//so cry to re-name aco_xxx....
#define co_go        ::base::CoroRunner::_go(__FUNCTION__, __FILE__, __LINE__)-
#define co_pause     ::base::CoroRunner::Runner().Yield()
#define co_resumer() ::base::CoroRunner::Runner().Resumer()
#define co_sleep(ms) ::base::CoroRunner::Runner().Sleep(ms)
#define co_can_yield \
  (base::MessageLoop::Current() && ::base::CoroRunner::Runner().CanYieldHere())

#endif
