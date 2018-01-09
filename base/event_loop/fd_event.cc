#include "fd_event.h"
#include "glog/logging.h"
#include "base/base_constants.h"

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
    VLOG(GLOG_VTRACE) << "update fdevent to poll, events:" << events_;
    delegate_->UpdateFdEvent(this);
  } else {
    LOG(INFO) << "No FdEvent::Delegate Can update this FdEvent, fd:" << fd();
  }
}

void FdEvent::SetCloseCallback(const EventCallback &cb) {
  close_callback_ = cb;
}

int FdEvent::fd() const {
  return fd_;
}

void FdEvent::HandleEvent() {
  VLOG(GLOG_VTRACE) << "FdEvent::handle_event: " << RcvEventAsString();

  /* in normal case, this caused by peer close fd */
  do {

    //can read and not peer half close
    if ((revents_ & (EPOLLIN | EPOLLPRI)) &&
        (!(revents_ & EPOLLRDHUP)) &&
        read_callback_) {
      read_callback_();
    }
    //write
    if ((revents_ & EPOLLOUT) && write_callback_) {
      write_callback_();
    }

    //peer close
    if (( (revents_ & EPOLLHUP) || ((revents_ & EPOLLIN) && (revents_ & EPOLLRDHUP)) ) &&
         close_callback_) {
      close_callback_();
    }
    //internal error or invailed fd
    if ( ((revents_ & EPOLLERR) || (revents_ & POLLNVAL)) && error_callback_) {
      error_callback_();
    }
    //EPOLLPRI

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
    oss << "NVAL ]";
  return oss.str().c_str();
}

} //namespace
