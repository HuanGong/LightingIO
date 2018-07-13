#include "event_pump.h"

#include "glog/logging.h"
#include "linux_signal.h"
#include "io_multiplexer_epoll.h"

namespace base {

EventPump::EventPump()
  : delegate_(NULL),
    running_(false) {
  multiplexer_.reset(new base::IoMultiplexerEpoll());
}

EventPump::EventPump(PumpDelegate* d)
  : delegate_(d),
    running_(false) {
  multiplexer_.reset(new base::IoMultiplexerEpoll());
}

EventPump::~EventPump() {
  multiplexer_.reset();
}

void EventPump::Run() {
  //sigpipe
  Signal::signal(SIGPIPE, []() { LOG(ERROR) << "sigpipe."; });

  if (delegate_) {
    delegate_->BeforePumpRun();
  }
  running_ = true;
  while (running_) {

    int32_t perfect_timeout = timer_queue_.HandleExpiredTimer();

    active_events_.clear();

    multiplexer_->WaitingIO(active_events_, perfect_timeout);

    for (auto& fd_event : active_events_) {
      fd_event->HandleEvent();
    }
  }

  running_ = false;

  if (delegate_) {
    delegate_->AfterPumpRun();
  }
}

void EventPump::Quit() {
  running_ = false;
}

bool EventPump::IsInLoopThread() const {
  return tid_ == std::this_thread::get_id();
}

QuitClourse EventPump::Quit_Clourse() {
  return std::bind(&EventPump::Quit, this);
}

bool EventPump::InstallFdEvent(FdEvent *fd_event) {
  CHECK(IsInLoopThread());
  if (fd_event->EventDelegate()) {
    LOG(ERROR) << "can't install event to eventPump twice or install event to two different eventPump";
    return false;
  }

  fd_event->SetDelegate(AsFdEventDelegate());

  multiplexer_->AddFdEvent(fd_event);
  return true;
}

bool EventPump::RemoveFdEvent(FdEvent* fd_event) {
  CHECK(IsInLoopThread());
  if (!fd_event->EventDelegate()) {
    LOG(ERROR) << "event don't attach to any Pump";
    return false;
  }
  fd_event->SetDelegate(nullptr);

  multiplexer_->DelFdEvent(fd_event);
  return true;
}

void EventPump::UpdateFdEvent(FdEvent* fd_event) {
  CHECK(IsInLoopThread() && fd_event->EventDelegate());
  multiplexer_->UpdateFdEvent(fd_event);
}

int32_t EventPump::ScheduleTimer(RefTimerEvent& timerevent) {
  CHECK(IsInLoopThread());
  return timer_queue_.AddTimerEvent(timerevent);
}

bool EventPump::CancelTimer(uint32_t timer_id) {
  CHECK(IsInLoopThread());
  return timer_queue_.CancelTimerEvent(timer_id);
}

}//end base
