#ifndef FD_EVENT_H
#define FD_EVENT_H

#include <memory>

#include <functional>
#include <sys/epoll.h>
#include <sys/poll.h>
#include "base/base_micro.h"

namespace base {

class FdEvent {
public:
  /* when a fdevent is changed or be modify be user, we need to notify the poller
     change the events we care about */
  class Delegate {
    public:
      virtual ~Delegate() {}
      virtual void UpdateFdEvent(FdEvent* fd_event) = 0;
  };

  typedef std::function<void()> EventCallback;
  typedef std::shared_ptr<FdEvent> RefFdEvent;

  static RefFdEvent create(int fd, uint32_t events) {
      return std::make_shared<FdEvent>(fd, events);
  }

  FdEvent(int fd, uint32_t events);
  ~FdEvent();

  void SetDelegate(Delegate* d);
  Delegate* EventDelegate() {return delegate_;}

  void HandleEvent();
  void SetReadCallback(const EventCallback &cb);
  void SetWriteCallback(const EventCallback &cb);
  void SetErrorCallback(const EventCallback &cb);
  void SetCloseCallback(const EventCallback &cb);

  int fd() const;
  //the things we take care about
  uint32_t MonitorEvents() const;
  void SetRcvEvents(uint32_t ev);

  void Update();
  void EnableReading() { events_ |=   EPOLLIN;  Update(); }
  void EnableWriting() { events_ |=   EPOLLOUT; Update();  }
  void EnableClosing() { events_ |=   EPOLLRDHUP; Update(); }
  void EnableError() { events_ |=   EPOLLERR; Update(); }

  void DisableError() { events_ &=  !EPOLLERR; Update();  }
  void DisableClosing() { events_ &=  !EPOLLRDHUP; Update();  }
  void DisableReading() { events_ &=  !EPOLLIN; Update();  }
  void DisableWriting() { events_ &=   !EPOLLOUT; Update();  }
  void DisableAll() {
      events_ &= !EPOLLOUT;
      events_ &= !EPOLLIN;
      events_ &= !EPOLLRDHUP;
      events_ &= !EPOLLERR;
      Update();
  }
  // for debug
  std::string RcvEventAsString();
  std::string MonitorEventAsString();
private:
  std::string events2string(uint32_t event);
  Delegate* delegate_;

  int fd_;
  uint32_t events_;
  uint32_t revents_;

  EventCallback read_callback_;
  EventCallback write_callback_;
  EventCallback error_callback_;
  EventCallback close_callback_;

  DISALLOW_COPY_AND_ASSIGN(FdEvent);
};
//export

} // namespace
#endif // EVENT_H
