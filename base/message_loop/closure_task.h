
#ifndef IO_EV_TASK_H_H
#define IO_EV_TASK_H_H

#include "base/base_micro.h"
#include "base/time_utils.h"

#include <list>
#include <memory>
#include <queue>
#include <assert.h>
#include "glog/logging.h"

struct event_base;
struct event;

namespace base {

class QueuedTask {
 public:
  QueuedTask() {}
  virtual ~QueuedTask() {}

  virtual bool Run() = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(QueuedTask);
};


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

template <class Closure, class Cleanup>
class ClosureTaskWithCleanup : public ClosureTask<Closure> {
 public:
  ClosureTaskWithCleanup(const Closure& closure, Cleanup cleanup)
      : ClosureTask<Closure>(closure), cleanup_(cleanup) {}
  ~ClosureTaskWithCleanup() { cleanup_(); }
 private:
  Cleanup cleanup_;
};

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
