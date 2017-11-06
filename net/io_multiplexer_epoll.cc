#include "io_multiplexer_epoll.h"

#include "glog/logging.h"
#include <assert.h>

namespace base {

const int kMax_EPOLL_FD_NUM = 65535;

IoMultiplexerEpoll::IoMultiplexerEpoll()
  : IoMultiplexer(),
    epoll_fd_(::epoll_create(kMax_EPOLL_FD_NUM)),
    ep_events_(kMax_EPOLL_FD_NUM) {
}

IoMultiplexerEpoll::~IoMultiplexerEpoll() {
}

int IoMultiplexerEpoll::WaitingIO(FdEventList& active_list, int timeout_ms) {

  int turn_active_count = ::epoll_wait(epoll_fd_,
                                       &(*ep_events_.begin()),
                                       kMax_EPOLL_FD_NUM,
                                       timeout_ms);

  if (turn_active_count < 0) {//error
    LOG(ERROR) << "epoll_wait return ERROR code:" << turn_active_count;
    return turn_active_count;
  }
  for (int idx = 0; idx < turn_active_count; idx++) {
    auto fd_event_pair = fdev_map_.find(ep_events_[idx].data.fd);
    assert(fd_event_pair != fdev_map_.end());
    FdEvent* pfd_ev = fd_event_pair->second;
    assert(pfd_ev->fd() == ep_events_[idx].data.fd);
    active_list.push_back(pfd_ev);
  }
  return turn_active_count;
}

void IoMultiplexerEpoll::AddFdEvent(FdEvent* fd_ev) {
  assert(fd_ev);
  epoll_add(fd_ev->fd(), fd_ev->events());
  fdev_map_[fd_ev->fd()] = fd_ev;
}

void IoMultiplexerEpoll::DelFdEvent(FdEvent* fd_ev) {
  assert(fd_ev);
  epoll_del(fd_ev->fd(), fd_ev->events());
  fdev_map_[fd_ev->fd()] = fd_ev;
}

void IoMultiplexerEpoll::UpdateFdEvent(FdEvent* fd_ev) {
  assert(fd_ev);
  epoll_mod(fd_ev->fd(), fd_ev->events());
  fdev_map_[fd_ev->fd()] = fd_ev;
}

void IoMultiplexerEpoll::epoll_del(int fd, uint32_t events) {
  this->epoll_ctl(fd, events, EPOLL_CTL_DEL);
}
void IoMultiplexerEpoll::epoll_add(int fd, uint32_t events) {
  this->epoll_ctl(fd, events, EPOLL_CTL_ADD);
}
void IoMultiplexerEpoll::epoll_mod(int fd, uint32_t events) {
  this->epoll_ctl(fd, events, EPOLL_CTL_MOD);
}
void IoMultiplexerEpoll::epoll_ctl(int fd, uint32_t events, int op) {
  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = events;
  ::epoll_ctl(epoll_fd_, op, fd, &ev);
}

}
