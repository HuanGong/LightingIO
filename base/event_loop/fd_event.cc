#include "fd_event.h"
#include "glog/logging.h"

namespace base {

FdEvent::FdEvent(int fd, uint32_t event):
  delegate_(NULL),
  fd_(fd),
  events_(event) {
}

FdEvent::~FdEvent() {
}

void FdEvent::SetDelegate(Delegate* d) {
  delegate_ = d;
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

void FdEvent::SetRcvEvents(uint32_t ev){
  revents_ = ev;
}

void FdEvent::Update() {
  if (delegate_) {
    delegate_->UpdateFdEvent(this);
  } else {
    VLOG(1) << "No FdEventDelegate Can update this FdEvent, fd:" << fd();
  }
}

void FdEvent::SetCloseCallback(const EventCallback &cb) {
  close_callback_ = cb;
}

int FdEvent::fd() const {
  return fd_;
}

void FdEvent::HandleEvent() {
  LOG(INFO) << "FdEvent::handle_event:" << revents_;
  if(revents_ & EPOLLIN && read_callback_) {
    VLOG(1) << "EPOLLIN";
    read_callback_();
  }

  if(revents_ & EPOLLOUT && write_callback_) {
    VLOG(1) << "EPOLLOUT";
    write_callback_();
  }

  if(revents_ & EPOLLERR && error_callback_) {
    VLOG(1) << "EPOLLERR";
    error_callback_();
  }

  if(revents_ & EPOLLRDHUP && close_callback_) {
    VLOG(1) << "EPOLLRDHUP";
    close_callback_();
  }
  //clear the revents_ avoid invoke twice
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
    oss << "NVAL ]";
  return oss.str().c_str();
}

} //namespace
