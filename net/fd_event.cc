#include "fd_event.h"
#include "glog/logging.h"

namespace base {

FdEvent::FdEvent(FdEventDelegate* d, int fd, uint32_t event):
  delegate_(d),
  fd_(fd),
  events_(event) {
}

FdEvent::~FdEvent() {
  delegate_->DelFdEventFromMultiplexors(this);
}

uint32_t FdEvent::events() const {
  return events_;
}

void FdEvent::set_read_callback(const EventCallback &cb) {
  read_callback_ = cb;
}

void FdEvent::set_write_callback(const EventCallback &cb) {
  write_callback_ = cb;
}

void FdEvent::set_error_callback(const EventCallback &cb) {
  error_callback_ = cb;
}

void FdEvent::set_revents(uint32_t ev){
  revents_ = ev;
}

void FdEvent::update() {
  delegate_->UpdateFdEventToMultiplexors(this);
}

void FdEvent::set_close_callback(const EventCallback &cb) {
  close_callback_ = cb;
}

int FdEvent::fd() const {
  return fd_;
}

void FdEvent::handle_event() {
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

} //namespace
