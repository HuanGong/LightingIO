#ifndef IO_EV_LOOP_H_H
#define IO_EV_LOOP_H_H

#include "event.h"
#include <atomic>
#include <thread>

#include <iostream>
#include <queue>

namespace IO {

enum LoopState {
  ST_STOP = 0,
  ST_INITING = 1,
  ST_INITTED = 2,
  ST_STARTED = 3
};

typedef std::function<void()> TimerFunctor;

class EvIOLoop {
  public:
    EvIOLoop();
    ~EvIOLoop();

    void start();
    void stop();
    int status();
    void pause();
    void resume();

    int StartTimer(const TimerFunctor& funtor);

    void Init();
  private:
    void OnReadFiFo() {
      std::cout << "OnreadFifo" << std::endl;
    }

    void test() {
      std::cout << "test run" << std::endl;
    }

    void test2(int i) {
      std::cout << "test run" << std::endl;
    }

    void run_watch_dog(int sock, short event, void *arg);

    event_base* ev_base_;
    std::atomic_int status_;
    std::thread::id tid_;
    struct event watchdog_evt_;

    int fd_fifo_;
    std::string fifo_name_;

    std::vector<TimerFunctor> timer_functor_;
};

} //endnamespace
#endif
