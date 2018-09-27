#ifndef BASE_TIMER_EVENT_H_
#define BASE_TIMER_EVENT_H_

#include <cinttypes>
#include "time/timestamp.h"
#include "closure/closure_task.h"

namespace base {

typedef std::unique_ptr<TaskBase> UniqueTimerTask;

class TimerEvent {
public:
  typedef std::shared_ptr<TimerEvent> RefTimerEvent;
  static RefTimerEvent CreateOneShotTimer(int64_t ms_later, UniqueTimerTask);
  static RefTimerEvent CreateRepeatedTimer(int64_t ms_later, UniqueTimerTask);

  TimerEvent(int64_t ms, bool once = true);
  TimerEvent(const Timestamp& t, bool once = true);
  ~TimerEvent();

  const Timestamp& Time() const;
  Timestamp& MutableTime() { return time_; }

  void Invoke();
  void SetTimerTask(UniqueTimerTask task);

  bool IsOneShotTimer() const {return once_;}
  int64_t Interval() const { return interval_;}
private:
  bool once_;

  Timestamp time_;
  int64_t interval_;

  UniqueTimerTask timer_task_;
};

typedef std::shared_ptr<TimerEvent> RefTimerEvent;

class TimerEventCompare {
public:
  bool operator()(const RefTimerEvent& lf, const RefTimerEvent& rt) {
    return lf->Time() > rt->Time();
  }
};

}//end base
#endif
