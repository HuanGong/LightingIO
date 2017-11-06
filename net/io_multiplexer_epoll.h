#ifndef BASE_IO_MULTIPLEXER_EPOLL_H
#define BASE_IO_MULTIPLEXER_EPOLL_H

#include "io_multiplexer.h"

namespace base {

class IoMultiplexerEpoll : public IoMultiplexer {
public:
  IoMultiplexerEpoll();
  ~IoMultiplexerEpoll();

  void AddFdEvent(FdEvent* fd_ev) override;
  void DelFdEvent(FdEvent* fd_ev) override;
  void UpdateFdEvent(FdEvent* fd_ev) override;

  int WaitingIO(FdEventList& active_list, int timeout_ms) override;
private:
  void epoll_del(int fd, uint32_t events);
  void epoll_add(int fd, uint32_t events);
  void epoll_mod(int fd, uint32_t events);
  void epoll_ctl(int fd, uint32_t events, int op);
private:
  int epoll_fd_;
  std::vector<epoll_event> ep_events_;
};

}
#endif
