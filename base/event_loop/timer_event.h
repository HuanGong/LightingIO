#ifndef BASE_TIMER_EVENT_H_
#define BASE_TIMER_EVENT_H_

#include <cinttypes>
#include "time/timestamp.h"
#include "closure/closure_task.h"

namespace base {

class TimerEvent {
public:
  typedef std::shared_ptr<TimerEvent> RefTimerEvent;
  typedef std::unique_ptr<QueuedTask> UniqueTimerTask;

  class Compare {
  public:
    bool operator()(const RefTimerEvent& lf, const RefTimerEvent& rt) {
      return lf->Time() > timeEvent2->Time();
    }
  };

public:
  TimerEvent(Timestamp t, bool once = true);
  ~TimerEvent();

  void SetTimerTask(UniqueTimerTask& task_);
  void Cancel() {
    valid_ = false;
  }
  void Invoke() {
    if (!valid_)
  }
private:
  bool once_;
  bool valid_;
  Timestamp time_;
  UniqueTimerTask timer_task_;
}



}//end base
#endif
