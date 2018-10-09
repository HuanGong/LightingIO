#ifndef BASE_CLOSURE_TASK_H_H
#define BASE_CLOSURE_TASK_H_H

#include <list>
#include <memory>
#include <queue>
#include <assert.h>
#include <functional>
#include "glog/logging.h"
#include "location.h"
#include "base/base_micro.h"
#include <atomic>

#define NewClosureAlias(Location, Functor) ::base::CreateClosure(Location, Functor)
#define NewClosureWithCallbackAlias(Location, Functor, Cleanup) ::base::CreateClosureWithCallback(Location, Functor, Cleanup)

#define NewClosure(Functor) NewClosureAlias(FROM_HERE, Functor)
#define NewClosureWithCleanup(Functor, Cleanup) NewClosureWithCallbackAlias(Location, Functor, Cleanup)

namespace base {

class TaskBase {
public:
  TaskBase() {}
  explicit TaskBase(const Location& location) : location_(location) {}
  virtual ~TaskBase() {}
  virtual void Run() = 0;
protected:
  std::string ClosureInfo() const {
    return location_.ToString();
  };
private:
  Location location_;
};


template <typename Functor>
class ClosureTask : public TaskBase {
public:
  explicit ClosureTask(const Location& location, const Functor& closure)
   : TaskBase(location),
     closure_task(closure) {
  }
  void Run() override {
    try {
      closure_task();
    } catch (...) {
      LOG(WARNING) << "Task Closure Exception, Task From:" << ClosureInfo();
    }
  }
private:
  Functor closure_task;
};

template <class Closure, class Cleanup>
class ClosureTaskWithCleanup : public ClosureTask<Closure> {
public:
  ClosureTaskWithCleanup(const Location& location, const Closure& closure, const Cleanup& cleanup)
      : ClosureTask<Closure>(location, closure),
        cleanup_task(cleanup) {
  }
  ~ClosureTaskWithCleanup() {
    //try {
      cleanup_task();
    //} catch (...) {
      LOG(WARNING) << "Cleanup Closure Exception, Task From:" << TaskBase::ClosureInfo();
    //}
  }
private:
  Cleanup cleanup_task;
};

template <class Closure>
static std::unique_ptr<TaskBase> CreateClosure(const Location& location, const Closure& closure) {
  return std::unique_ptr<TaskBase>(new ClosureTask<Closure>(location, closure));
}

template <class Closure, class Cleanup>
static std::unique_ptr<TaskBase> CreateClosureWithCallback(const Location& location,
                                                           const Closure& closure,
                                                           const Cleanup& cleanup) {
  return std::unique_ptr<TaskBase>(new ClosureTaskWithCleanup<Closure, Cleanup>(location, closure, cleanup));
}

typedef std::function<void()> SigHandler;
typedef std::function<void()> StlClosure;
typedef std::unique_ptr<TaskBase> ClosurePtr;

}// end namespace
#endif
