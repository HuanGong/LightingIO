#ifndef _BASE_EVENTLOOP_TIMER_QUEUE_H_
#define _BASE_EVENTLOOP_TIMER_QUEUE_H_

#include <vector>
#include <stack>
#include "timer_event.h"

namespace base {

typedef struct {
  Timestamp time_;
  /*index in timer_events_*/
  uint32_t index_;
} TimerEntry;

class TimerEntryCompare {
public:
  bool operator()(const TimerEntry& lf, const TimerEntry& rt) {
    return lf.time_ > rt.time_;
  }
};

class TimerTaskQueue {
public:
  TimerTaskQueue() {};
  ~TimerTaskQueue() {};

  /*return the timer_index as identify for cancel*/
  uint32_t AddTimerEvent(RefTimerEvent& timer_event);

  /*return true if cancel success*/
  bool CancelTimerEvent(uint32_t timer_id);

  /* return the duration(millsecond) for next timerevent triggle*/
  uint64_t HandleExpiredTimer();
  void TestingPriorityQueue();
private:

  void InvokeAndReScheduleIfNeed(uint32_t timerid);
  int64_t CalculateNextTimerDuration();

  typedef std::priority_queue<TimerEntry,
                              std::vector<TimerEntry>,
                              TimerEntryCompare> TimerIndexHeap;

  TimerIndexHeap timer_heap_;
  std::vector<RefTimerEvent> timer_events_;
  std::stack<uint32_t> reusable_timerid_;
};

}
#endif
