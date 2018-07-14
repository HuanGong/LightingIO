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
  ::close(epoll_fd_);
}

int IoMultiplexerEpoll::WaitingIO(FdEventList& active_list, int32_t timeout_ms) {

  int turn_active_count = ::epoll_wait(epoll_fd_,
                                       &(*ep_events_.begin()),
                                       kMax_EPOLL_FD_NUM,
                                       timeout_ms);

  if (turn_active_count < 0) {//error
    int32_t err = errno;
    LOG(ERROR) << "epoll_wait ERROR" << strerror(err);
    return 0;
  }

  for (int idx = 0; idx < turn_active_count; idx++) {

    FdEvent* fd_ev = (FdEvent*)(ep_events_[idx].data.ptr);
    CHECK(fd_ev);

    fd_ev->SetRcvEvents(ep_events_[idx].events);
    active_list.push_back(fd_ev);
  }
  return turn_active_count;
}

void IoMultiplexerEpoll::AddFdEvent(FdEvent* fd_ev) {
  EpollCtl(fd_ev, EPOLL_CTL_ADD);
  if (listen_events_.Attatched(fd_ev)) {
    listen_events_.remove(fd_ev);
  }
  listen_events_.push_back(fd_ev);
}

void IoMultiplexerEpoll::DelFdEvent(FdEvent* fd_ev) {

  EpollCtl(fd_ev, EPOLL_CTL_DEL);
  if (listen_events_.Attatched(fd_ev)) {
    listen_events_.remove(fd_ev);
  } else {
    LOG(ERROR) << "Attept Remove A FdEvent From Another Holder";
  }
}

void IoMultiplexerEpoll::UpdateFdEvent(FdEvent* fd_ev) {
  if (!listen_events_.Attatched(fd_ev)) {
    listen_events_.push_back(fd_ev);
  }
  EpollCtl(fd_ev, EPOLL_CTL_MOD);
}

int IoMultiplexerEpoll::EpollCtl(FdEvent* fdev, int opt) {
  struct epoll_event ev;

  int fd = fdev->fd();
  ev.data.ptr = fdev;
  ev.events = fdev->MonitorEvents();

  int ret = ::epoll_ctl(epoll_fd_, opt, fd, &ev);
  if (ret != 0) {
    int32_t e = errno;
    LOG(ERROR) << "epoll_ctl error:" << EpollOptToString(opt) << " fd:" << fd << " events:" << ev.events << " errno:" << e;
    CHECK(false);
  }
  return 0;
}

std::string IoMultiplexerEpoll::EpollOptToString(int opt) {
  switch(opt) {
    case EPOLL_CTL_ADD:
      return "EPOLL_CTL_ADD";
    case EPOLL_CTL_DEL:
      return "EPOLL_CTL_DEL";
    case EPOLL_CTL_MOD:
      return "EPOLL_CTL_MOD";
    default:
      break;
  }
  return "BAD CTL OPTION";
}

}
