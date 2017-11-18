#ifndef FD_EVENT_H
#define FD_EVENT_H

#include <memory>

#include <functional>
#include <sys/epoll.h>
#include "base/base_micro.h"

namespace base {

class FdEvent {
public:
  typedef std::shared_ptr<FdEvent> RefFdEvent;

  /* when a fdevent is changed or be modify be user, we need to notify the poller
     change the events we care about */
  class FdEventDelegate {
    public:
      virtual ~FdEventDelegate() {}
      //virtual void AddFdEventToMultiplexors(FdEvent* fd_event) = 0;
      //virtual void DelFdEventFromMultiplexors(FdEvent* fd_event) = 0;
      virtual void UpdateFdEventToMultiplexors(FdEvent* fd_event) = 0;
  };

  typedef std::function<void()> EventCallback;
  typedef std::shared_ptr<FdEvent> FdEventPtr;

  static FdEventPtr create(int fd, uint32_t events) {
      return std::make_shared<FdEvent>(fd, events);
  }

  FdEvent(int fd, uint32_t events);
  ~FdEvent();

  void set_delegate(FdEventDelegate* d);
  FdEventDelegate* delegate() {return delegate_;}

  void handle_event();
  void set_read_callback(const EventCallback &cb);
  void set_write_callback(const EventCallback &cb);
  void set_error_callback(const EventCallback &cb);
  void set_close_callback(const EventCallback &cb);

  int fd() const;

  //the things we take care about
  uint32_t events() const;

  //current active events
  void set_revents(uint32_t ev);

  void update();
  void enable_reading() { events_ |=   EPOLLIN;  update(); }
  void enable_writing() { events_ |=   EPOLLOUT; update();  }
  void enable_closing() { events_ |=   EPOLLRDHUP; update(); }
  void disable_closing() { events_ &=  !EPOLLRDHUP; update();  }
  void enable_error() { events_ |=   EPOLLERR; update(); }
  void disable_error() { events_ &=  !EPOLLERR; update();  }
  void disable_reading() { events_ &=  !EPOLLIN; update();  }
  void disable_writing() { events_ &=   !EPOLLOUT; update();  }
  void disable_all() {
      events_ &= !EPOLLOUT;
      events_ &= !EPOLLIN;
      events_ &= !EPOLLRDHUP;
      events_ &= !EPOLLERR;
      update();
  }

private:
  FdEventDelegate* delegate_;

  int fd_;
  uint32_t events_;
  uint32_t revents_;

  EventCallback read_callback_;
  EventCallback write_callback_;
  EventCallback error_callback_;
  EventCallback close_callback_;

  DISALLOW_COPY_AND_ASSIGN(FdEvent);
};

} // namespace
#endif // EVENT_H
