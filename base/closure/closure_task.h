#ifndef BASE_CLOSURE_TASK_H_H
#define BASE_CLOSURE_TASK_H_H

#include <list>
#include <memory>
#include <queue>
#include <assert.h>
#include <functional>
#include "glog/logging.h"

#include "base/base_micro.h"

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
/*
template<typename _Func, typename... _BoundArgs>
static std::unique_ptr<QueuedTask> NewClosure(_Func&& __f, _BoundArgs&&... __args) {
  auto f = std::bind(std::forward<_Func>(__f), std::forward<_BoundArgs>(__args)...);
  return std::unique_ptr<QueuedTask>(new ClosureTask(std::move(f)));
}*/

typedef std::function<void()> SigHandler;
typedef std::function<void()> StlClourse;
typedef std::function<void()> StlClosure;

}// end namespace
#endif
