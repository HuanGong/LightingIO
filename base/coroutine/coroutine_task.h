#ifndef COROUTINE_TASK_H_
#define COROUTINE_TASK_H_

#include <memory>
#include <assert.h>

#include <functional>
#include "glog/logging.h"
#include "base/base_micro.h"
#include "base/time/time_utils.h"

namespace base {

typedef std::function<void()> CoroClosure;

//deprecated and discarded
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

}//end base
#endif
