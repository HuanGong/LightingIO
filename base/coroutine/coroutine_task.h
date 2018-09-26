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

}//end base
#endif
