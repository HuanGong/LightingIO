#include "event_pump.h"

#include "glog/logging.h"
#include "linux_signal.h"
#include "io_multiplexer_epoll.h"

namespace base {

EventPump::EventPump()
  : running_(false),
    prefect_timeout_(1) {
    multiplexer_.reset(new base::IoMultiplexerEpoll());
}

EventPump::~EventPump() {
  multiplexer_.reset();
}

void EventPump::Run() {
  //sigpipe
  Signal::signal(SIGPIPE, []() { LOG(ERROR) << "sigpipe."; });

  running_ = true;

  while (running_) {

    prefect_timeout_ = timer_queue_.HandleExpiredTimer();

    active_events_.clear();
    LOG(INFO) << "Waiting For Epoll Event With timeout:" << prefect_timeout_ << " ms";
    multiplexer_->WaitingIO(active_events_, prefect_timeout_);

    for (auto& fd_event : active_events_) {
      fd_event->HandleEvent();
    }
  }
  running_ = false;
}

void EventPump::Quit() {
  running_ = false;
}

QuitClourse EventPump::Quit_Clourse() {
  return std::bind(&EventPump::Quit, this);
}

void EventPump::InstallFdEvent(FdEvent *fd_event) {
  //fd_event->set_delegate(NULL);
  //fd_event->set_delegate(this);
  multiplexer_->AddFdEvent(fd_event);
}

void EventPump::RemoveFdEvent(FdEvent* fd_event) {
  multiplexer_->DelFdEvent(fd_event);
}

void EventPump::UpdateFdEvent(FdEvent* fd_event) {
  multiplexer_->UpdateFdEvent(fd_event);
}

int32_t EventPump::ScheduleTimer(RefTimerEvent& timerevent) {
  return timer_queue_.AddTimerEvent(timerevent);
}

bool EventPump::CancelTimer(uint32_t timer_id) {
  return timer_queue_.CancelTimerEvent(timer_id);
}

}//end base
