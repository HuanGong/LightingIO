
#include "ev_loop.h"

#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <event2/util.h>

#include <limits.h>
#include <string>
#include <sstream>

#include <iostream>

namespace IO {

void onTime(int sock, short event, void *arg) {
  static int cnt = 0;

  std::cout << "callback on threadId:" << std::this_thread::get_id() << std::endl;
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
}

EvIOLoop::EvIOLoop() :
  ev_base_(NULL),
  fd_fifo_(0) {

  //Init();
}

EvIOLoop::~EvIOLoop() {

}

void EvIOLoop::Init() {
  struct timeval tv;

  ev_base_ = event_base_new();
  tid_ = std::this_thread::get_id();

  event_assign(&watchdog_evt_,
               ev_base_,
               -1,
               EV_PERSIST,
               onTime,
               (void*)& watchdog_evt_);

  evutil_timerclear(&tv);
  tv.tv_sec = 2;
  event_add(&watchdog_evt_, &tv);

  // init fifi using for manager task
  mode_t mode = 0666;
  std::stringstream ss;
  ss << tid_;
  fifo_name_ = "/tmp/lightingio_" + ss.str();
  if ((fd_fifo_ = ::mkfifo(fifo_name_.c_str(), mode )) < 0) {
    perror ( "failed to mkfifo" );
    exit ( 1 );
  }
  std::cout << "successfully create FIFO in path:" << fifo_name_ << std::endl;
}

void EvIOLoop::start() {
  std::cout << "started" << tid_ << std::endl;
  event_base_dispatch(ev_base_);
}

void EvIOLoop::stop() {
}

int EvIOLoop::status() {
  return 0;
}

void EvIOLoop::pause() {

}

void EvIOLoop::resume() {

}

}
