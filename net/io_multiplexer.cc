
#include "io_multiplexer.h"

namespace base {

IoMultiplexer::IoMultiplexer() {

}
IoMultiplexer::~IoMultiplexer() {

}

uint32_t IoMultiplexer::WatchingFdCounts() const {
  return fdev_map_.size();
}

bool IoMultiplexer::HasFdEvent(const FdEvent* fd_ev) {
  FdEventMap::const_iterator it = fdev_map_.find(fd_ev->fd());
  return it != fdev_map_.end() && it->second == fd_ev;
}

}
