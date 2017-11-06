#ifndef BASE_RUNLOOP_H_
#define BASE_RUNLOOP_H_

#include "io_multiplexer_epoll.h"

#include <vector>

namespace base {

class RunLoop() {
public:
  RunLoop();
  ~RunLoop();

  void Run();
  void Quit();

  bool Running() { return running_; }
private:
  bool running_;
  IoMultiplexer* io_multiplexer_;
  std::vector<FdEvent*> active_events_;
};

}
#endif
