
#include "timer_event.h"
#include "glog/logging.h"

namespace base {

TimerEvent::TimerEvent(const Timestamp& t, bool once)
  : once_(once),
    time_(t),
    interval_(0) {
  Timestamp now = Timestamp::Now();

  interval_ = t.AsMillsecond() - now.AsMillsecond();

  LOG_IF(INFO, time_ < now) << " Init A Invalid (out date) TimerEvent";

  if (interval_ < 0) {
    interval_ = (-interval_);
  }
}

TimerEvent::TimerEvent(int64_t ms, bool once)
  : once_(once),
    time_(Timestamp::NMillisecondLater(ms)),
    interval_(ms) {
}

TimerEvent::~TimerEvent() {
  LOG(INFO) << "TimerEvent Gone";
}

const Timestamp& TimerEvent::Time() const {
  return time_;
}

void TimerEvent::SetTimerTask(UniqueTimerTask task) {
  timer_task_ = std::move(task);
}

void TimerEvent::Invoke() {
  if (timer_task_) {
    LOG(INFO) << "INVOKE Timer Task";
    timer_task_->Run();
  }
}

}
