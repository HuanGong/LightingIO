#ifndef COROUTINE_TASK_H_
#define COROUTINE_TASK_H_

#include <memory>
#include <assert.h>

#include "glog/logging.h"
#include "base/base_micro.h"
#include "base/time/time_utils.h"

namespace base {

class CoroTask {
 public:
  CoroTask() {}
  virtual ~CoroTask() {
  }

  virtual bool RunCoro() = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(CoroTask);
};

template <class Closure>
class CoroClosureTask : public CoroTask {
 public:
  explicit CoroClosureTask(const Closure& closure)
    : closure_(closure) {
  }
 private:
  bool RunCoro() override {
    closure_();
    return true;
  }
  Closure closure_;
};

template <class Closure>
static std::unique_ptr<CoroTask> NewCoroTask(const Closure& closure) {
  return std::unique_ptr<CoroTask>(new CoroClosureTask<Closure>(closure));
}
/*
template <class Functor, class Ctx>
class CoroCtxTask : public CoroTask {
public:
  explicit CoroCtxTask(const Functor& func,
                       std::unique_ptr<Ctx> ctx)
    : func_(func),
      ctx_(ctx) {
    }
private:
  bool RunCoro() override {
    closure_(ctx.get());
    return true;
  }
  Functor func_;
  std::unique_ptr<Ctx> ctx;
};
//CoroCtxTask<std::function<void(HttpMessage)> >
*/

}//end base
#endif
