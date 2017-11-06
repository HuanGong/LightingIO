#ifndef BASE_IO_MULTIPLEXER_H
#define BASE_IO_MULTIPLEXER_H

#include <map>
#include <vector>

#include "fd_event.h"

namespace base {

typedef std::vector<FdEvent*> FdEventList;

class IoMultiplexer {
public:
  IoMultiplexer();
  virtual ~IoMultiplexer();

  virtual void AddFdEvent(FdEvent* fd_ev);
  virtual void DelFdEvent(FdEvent* fd_ev);
  virtual void UpdateFdEvent(FdEvent* fd_ev);

  virtual int WaitingIO(FdEventList& active_list, int timeout_ms) = 0;

  uint32_t WatchingFdCounts() const;
  bool HasFdEvent(const FdEvent* fd_ev);
protected:
  typedef std::map<int, FdEvent*> FdEventMap;
  FdEventMap fdev_map_;

private:
  DISALLOW_COPY_AND_ASSIGN(IoMultiplexer);
};

}
#endif
