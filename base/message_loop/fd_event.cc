#include "fd_event.h"
#include "glog/logging.h"
#include "base/base_constants.h"

namespace base {

FdEvent::FdEvent(int fd, uint32_t event):
  watcher_(NULL),
  fd_(fd),
  events_(event),
  owner_fd_life_(true) {
}

FdEvent::~FdEvent() {
  if (owner_fd_life_) {
    ::close(fd_);
  }
}

void FdEvent::SetDelegate(FdEventWatcher* d) {
  watcher_ = d;
}

uint32_t FdEvent::MonitorEvents() const {
  return events_;
}

void FdEvent::SetReadCallback(const EventCallback &cb) {
  read_callback_ = cb;
}

void FdEvent::SetWriteCallback(const EventCallback &cb) {
  write_callback_ = cb;
}

void FdEvent::SetErrorCallback(const EventCallback &cb) {
  error_callback_ = cb;
}

void FdEvent::SetRcvEvents(uint32_t ev) {
  revents_ = ev;
}

void FdEvent::ResetCallback() {
  static const EventCallback kNullCallback;
  error_callback_ = kNullCallback;
  write_callback_ = kNullCallback;
  read_callback_  = kNullCallback;
  close_callback_ = kNullCallback;
}

void FdEvent::Update() {
  if (watcher_) {
    VLOG(GLOG_VTRACE) << "update fdevent to poll, events:" << events_;
    watcher_->OnEventChanged(this);
  } else {
    LOG(INFO) << "This fdevent not attach to any event dump yet, fd:" << fd();
  }
}

void FdEvent::SetCloseCallback(const EventCallback &cb) {
  close_callback_ = cb;
}

void FdEvent::HandleEvent() {
  VLOG(GLOG_VTRACE) << "FdEvent::handle_event: " << RcvEventAsString();
  do {
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
      if (close_callback_) {
        close_callback_();
      }
    }

    if (revents_ & (EPOLLERR | POLLNVAL)) {
      if (error_callback_) {
        error_callback_();
      }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
      if (read_callback_) {
        read_callback_();
      }
    }

    if ((revents_ & EPOLLOUT) && write_callback_) {
      write_callback_();
    }

  } while(0);
  revents_ = 0;
}

std::string FdEvent::RcvEventAsString() {
  return events2string(revents_);
}

std::string FdEvent::MonitorEventAsString() {
  return events2string(events_);
}
std::string FdEvent::events2string(uint32_t events) {
  std::ostringstream oss;
  oss << "fd: " << fd_ << ": [";
  if (events & EPOLLIN)
    oss << "IN ";
  if (events & EPOLLPRI)
    oss << "PRI ";
  if (events & EPOLLOUT)
    oss << "OUT ";
  if (events & EPOLLHUP)
    oss << "HUP ";
  if (events & EPOLLRDHUP)
    oss << "RDHUP ";
  if (events & EPOLLERR)
    oss << "ERR ";
  if (events & POLLNVAL)
    oss << "NVAL";
  oss << "]";
  return oss.str().c_str();
}

} //namespace
