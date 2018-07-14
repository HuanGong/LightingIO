#ifndef BASE_IO_MULTIPLEXER_H
#define BASE_IO_MULTIPLEXER_H

#include <map>
#include <vector>

#include "fd_event.h"
#include "queue/double_linked_list.h"

namespace base {

typedef std::vector<FdEvent*> FdEventList;

class IoMultiplexer {
public:
  IoMultiplexer();
  virtual ~IoMultiplexer();

  virtual void AddFdEvent(FdEvent* fd_ev) {};
  virtual void DelFdEvent(FdEvent* fd_ev) {};
  virtual void UpdateFdEvent(FdEvent* fd_ev) {};

  virtual int WaitingIO(FdEventList& active_list, int32_t timeout_ms) = 0;

  uint32_t WatchingFdCounts() const;
protected:
  DLinkedList<FdEvent> listen_events_;

private:
  DISALLOW_COPY_AND_ASSIGN(IoMultiplexer);
};

}
#endif
