
#include "io_multiplexer.h"

namespace base {

IoMultiplexer::IoMultiplexer() {
}
IoMultiplexer::~IoMultiplexer() {
}

uint32_t IoMultiplexer::WatchingFdCounts() const {
  return listen_events_.Size();
}

}
