#include "io_multiplexer_epoll.h"

#include "glog/logging.h"
#include <base/utils/sys_error.h>

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
                                       &ep_events_[0],
                                       kMax_EPOLL_FD_NUM,
                                       timeout_ms);

  if (turn_active_count < 0) {//error
    int32_t err = errno;
    LOG(ERROR) << "epoll fd:" << epoll_fd_ << " epoll_wait failed with error:" << StrError(err);
    return 0;
  }

  for (int idx = 0; idx < turn_active_count; idx++) {
    struct epoll_event& ev = ep_events_[idx];

    FdEvent* fd_ev = (FdEvent*)(ev.data.ptr);
    DCHECK(fd_ev);

    fd_ev->SetRcvEvents(ToLtEvent(ev.events));
    active_list.push_back(fd_ev);
  }
  return turn_active_count;
}

LtEvent IoMultiplexerEpoll::ToLtEvent(const uint32_t epoll_ev) {
  LtEvent event = LtEv::LT_EVENT_NONE;

  if (epoll_ev & EPOLLERR) {
    event |=  LtEv::LT_EVENT_ERROR;
  }
  //case readable:
  //case hang out: but can read till EOF
  if (epoll_ev & EPOLLHUP || epoll_ev & EPOLLIN) {
    event |=  LtEv::LT_EVENT_READ;
  }
  //writable
  if (epoll_ev & EPOLLOUT) {
    event |=  LtEv::LT_EVENT_WRITE;
  }
  //peer close
  if (epoll_ev & EPOLLRDHUP) {
    event |= LtEv::LT_EVENT_CLOSE;
  }
  return event;
}

uint32_t IoMultiplexerEpoll::ToEpollEvent(const LtEvent& lt_ev, bool add_extr) {
  uint32_t epoll_ev = 0;
  if (lt_ev & LtEv::LT_EVENT_READ) {
    epoll_ev |= EPOLLIN;
  }
  if (lt_ev & LtEv::LT_EVENT_WRITE) {
    epoll_ev |= EPOLLOUT;
  }
  if ((lt_ev & LtEv::LT_EVENT_CLOSE) || add_extr) {
    epoll_ev |= EPOLLRDHUP;
  }
  return epoll_ev;
}

void IoMultiplexerEpoll::AddFdEvent(FdEvent* fd_ev) {

  EpollCtl(fd_ev, EPOLL_CTL_ADD);

  if (listen_events_.Attatched(fd_ev)) {
    listen_events_.Remove(fd_ev);
  }
  listen_events_.PushBack(fd_ev);
}

void IoMultiplexerEpoll::DelFdEvent(FdEvent* fd_ev) {

  EpollCtl(fd_ev, EPOLL_CTL_DEL);

  if (listen_events_.Attatched(fd_ev)) {
    listen_events_.Remove(fd_ev);
  } else {
    LOG(ERROR) << "Attept Remove A FdEvent From Another Holder";
  }
}

void IoMultiplexerEpoll::UpdateFdEvent(FdEvent* fd_ev) {
  if (0 == EpollCtl(fd_ev, EPOLL_CTL_MOD)) {
    if (!listen_events_.Attatched(fd_ev)) {
      listen_events_.PushBack(fd_ev);
    }
  }
}

int IoMultiplexerEpoll::EpollCtl(FdEvent* fdev, int opt) {
  struct epoll_event ev;

  int fd = fdev->fd();
  ev.data.ptr = fdev;
  ev.events = ToEpollEvent(fdev->MonitorEvents());

  int ret = ::epoll_ctl(epoll_fd_, opt, fd, &ev);

  LOG_IF(ERROR, ret != 0) << "apply epoll_ctl opt " << EpollOptToString(opt)
    << " on fd " << fd << " failed, errno:" << StrError(errno)
    << " events:" << fdev->MonitorEventAsString();

  return ret;
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
