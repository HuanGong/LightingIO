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
}

void EventPump::Run() {
  //sigpipe
  Signal::signal(SIGPIPE, []() { LOG(ERROR) << "sigpipe."; });

  running_ = true;

  while (running_) {
    //Schedule Timer Task
    ScheduleTimerTask();

    active_events_.clear();
    //LOG(INFO) << "Start WaitingIO";
    multiplexer_->WaitingIO(active_events_, /*prefect_timeout_*/ 5000);
    //LOG(INFO) << "Somthing Happed On IO";
    for (auto& fd_event : active_events_) {
      fd_event->handle_event();
    }
  }
}

void EventPump::Quit() {
  running_ = false;
}

QuitClourse EventPump::Quit_Clourse() {
  return std::bind(&EventPump::Quit, this);
}

void EventPump::ScheduleTimerTask() {

}

void EventPump::InstallFdEvent(FdEvent *fd_event) {
  //fd_event->set_delegate(NULL);
  //fd_event->set_delegate(this);
  multiplexer_->AddFdEvent(fd_event);
}


}//end base
