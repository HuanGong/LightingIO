#include "timer_task_queue.h"
#include <glog/logging.h>

namespace base {

TimerTaskQueue::~TimerTaskQueue() {
  timer_events_.clear();
}

uint32_t TimerTaskQueue::AddTimerEvent(RefTimerEvent& timer_event) {

  uint32_t timer_id = 0;

  const Timestamp& timestamp_ = timer_event->Time();

  if (reusable_timerid_.empty()) {
    //timer_events_.push_back(std::move(timer_event));
    timer_events_.push_back(timer_event);
    timer_id = (timer_events_.size() - 1);
  } else {
    timer_id = reusable_timerid_.top();
    reusable_timerid_.pop();

    assert(timer_id < timer_events_.size());
    timer_events_[timer_id] = timer_event;
  }

  TimerEntry entry = {timestamp_, timer_id};
  timer_heap_.push(entry);

  return timer_id;
}

bool TimerTaskQueue::CancelTimerEvent(uint32_t timer_index) {
  if (timer_index >= timer_events_.size()) {
    LOG(INFO) << "Cancel a not exsist timerevent";
    return false;
  }

  auto& timerevent = timer_events_[timer_index];
  timerevent.reset();
  reusable_timerid_.push(timer_index);
}

uint64_t TimerTaskQueue::HandleExpiredTimer() {

  Timestamp now = Timestamp::Now();

  while(!timer_heap_.empty() &&
        timer_heap_.top().time_.AsMillsecond() <= now.AsMillsecond()) {

    const TimerEntry& top = timer_heap_.top();

    uint32_t timerid = top.index_;
    timer_heap_.pop();

    assert(timer_events_.size() > timerid);

    RefTimerEvent& timerevent = timer_events_[timerid];
    if (timerevent == NULL) { //has been canceled
      continue;
    }
    InvokeAndReScheduleIfNeed(timerid);
  }

  return CalculateNextTimerDuration();
}

int64_t TimerTaskQueue::CalculateNextTimerDuration() {
  if (timer_heap_.empty()) {
    LOG(INFO) << "No Timer TimerEvent Need Schedule Now";
    return 2000;
  }
  const auto& next_enter = timer_heap_.top();

  int64_t now_ms = Timestamp::Now().AsMillsecond();

  int64_t ms_for_next_timer_task = next_enter.time_.AsMillsecond() - now_ms;

  LOG_IF(ERROR, ms_for_next_timer_task < 0) << "A Bad Waiting Timeout Time";
  return ms_for_next_timer_task > 0 ? ms_for_next_timer_task : 0;
}

void TimerTaskQueue::InvokeAndReScheduleIfNeed(uint32_t timer_id) {
  //copy it, must do it, inboke should at the last to avoid be cancel or delete
  RefTimerEvent timerevent = timer_events_[timer_id];
  assert(timerevent);

  if (timerevent->IsOneShotTimer()) {
    //"fake delete" timer from vector
    timer_events_[timer_id].reset();
    assert(timer_events_[timer_id] == NULL);
    //mark as resusable
    reusable_timerid_.push(timer_id);
  } else { //this is a repeat timer, reschedule it again

    uint64_t interval_ms = timerevent->Interval();
    Timestamp& new_time = timerevent->MutableTime();

    new_time = Timestamp::NMillisecondLater(interval_ms);
    assert(new_time > Timestamp::Now());
    //install to priority queue
    TimerEntry entry = {new_time, timer_id};
    timer_heap_.push(entry);
  }

  /* ensure this invoke call at last,
   * bz the timer callback may delete or canle this timer*/
  timerevent->Invoke();
}

void TimerTaskQueue::TestingPriorityQueue() {

  TimerEntry entry10 = {Timestamp::NMicrosecondLater(10), 1};
  timer_heap_.push(entry10);

  TimerEntry entry20 = {Timestamp::NMicrosecondLater(20), 3};
  timer_heap_.push(entry20);

  TimerEntry entry14 = {Timestamp::NMicrosecondLater(14), 2};
  timer_heap_.push(entry14);

  const TimerEntry& entry = timer_heap_.top();
  LOG_IF(INFO, entry.time_ != entry10.time_) << "Test Failed";
}

}// end namespace base
