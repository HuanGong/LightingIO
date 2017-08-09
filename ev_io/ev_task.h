
#ifndef IO_EV_TASK_H_H
#define IO_EV_TASK_H_H

#include "base/base_micro.h"

#include <list>
#include <memory>
#include <queue>
#include <assert.h>

struct event_base;
struct event;

namespace IO {

#define DCHECK(condition)                                                               \
  do {                                                                                  \
    if (!(condition)) {                                                                 \
      std::cout << __FILE__ << __LINE__ << "DCHECK failed:" << #condition << std::endl; \
      assert(condition);                                                                \
    }                                                                                   \
  } while (0)

// Base interface for asynchronously executed tasks.
// The interface basically consists of a single function, Run(), that executes
// on the target queue.  For more details see the Run() method and TaskQueue.
class QueuedTask {
 public:
  QueuedTask() {}
  virtual ~QueuedTask() {}

  // Main routine that will run when the task is executed on the desired queue.
  // The task should return |true| to indicate that it should be deleted or
  // |false| to indicate that the queue should consider ownership of the task
  // having been transferred.  Returning |false| can be useful if a task has
  // re-posted itself to a different queue or is otherwise being re-used.
  virtual bool Run() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(QueuedTask);
};

class SetTimerTask : public QueuedTask {
 public:
  SetTimerTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds)
      : task_(std::move(task)),
        milliseconds_(milliseconds),
        posted_(0/*Time32()*/) {}

 private:
  bool Run() override {
    // Compensate for the time that has passed since construction
    // and until we got here.
    //uint32_t post_time = Time32() - posted_;
    //TaskQueue::Current()->PostDelayedTask(
    //    std::move(task_),
    //    post_time > milliseconds_ ? 0 : milliseconds_ - post_time);
    return true;
  }

  std::unique_ptr<QueuedTask> task_;
  const uint32_t milliseconds_;
  const uint32_t posted_;
};

// Simple implementation of QueuedTask for use with rtc::Bind and lambdas.
template <class Closure>
class ClosureTask : public QueuedTask {
 public:
  explicit ClosureTask(const Closure& closure) : closure_(closure) {}

 private:
  bool Run() override {
    closure_();
    return true;
  }

  Closure closure_;
};

// Extends ClosureTask to also allow specifying cleanup code.
// This is useful when using lambdas if guaranteeing cleanup, even if a task
// was dropped (queue is too full), is required.
template <class Closure, class Cleanup>
class ClosureTaskWithCleanup : public ClosureTask<Closure> {
 public:
  ClosureTaskWithCleanup(const Closure& closure, Cleanup cleanup)
      : ClosureTask<Closure>(closure), cleanup_(cleanup) {}
  ~ClosureTaskWithCleanup() { cleanup_(); }

 private:
  Cleanup cleanup_;
};

// Convenience function to construct closures that can be passed directly
// to methods that support std::unique_ptr<QueuedTask> but not template
// based parameters.
template <class Closure>
static std::unique_ptr<QueuedTask> NewClosure(const Closure& closure) {
  return std::unique_ptr<QueuedTask>(new ClosureTask<Closure>(closure));
}

template <class Closure, class Cleanup>
static std::unique_ptr<QueuedTask> NewClosure(const Closure& closure,
                                              const Cleanup& cleanup) {
  return std::unique_ptr<QueuedTask>(
      new ClosureTaskWithCleanup<Closure, Cleanup>(closure, cleanup));
}

}// end namespace
#endif
