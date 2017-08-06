#ifndef IO_EV_LOOP_H_H
#define IO_EV_LOOP_H_H

#include "event.h"
#include <atomic>
#include <thread>

#include <queue>

namespace IO {

enum LoopState {
  ST_STOP = 0,
  ST_INITING = 1,
  ST_INITTED = 2,
  ST_STARTED = 3
};

class EvIOLoop {
  public:
    EvIOLoop();
    ~EvIOLoop();

    void start();
    void stop();
    int status();
    void pause();
    void resume();

    void Init();
  private:
    event_base* ev_base_;
    std::atomic_int status_;
    std::thread::id tid_;
    struct event watchdog_evt_;

    int fd_fifo_;
    std::string fifo_name_;

    
};

} //endnamespace
#endif
