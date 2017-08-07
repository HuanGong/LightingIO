
#include "ev_loop.h"

#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <event2/util.h>

#include <limits.h>
#include <string>
#include <sstream>
#include <functional>

#include <base/utils/hpp_2_c_helper.h>

#include <iostream>

namespace IO {

typedef void (*libevent_callback_t)(evutil_socket_t, short, void*);

void fifo_read(evutil_socket_t fd, short event, void *arg) {
  std::cout << __FUNCTION__ << std::endl;
  std::cout << "sockfd:" << fd << std::endl;
}


EvIOLoop::EvIOLoop() :
  ev_base_(NULL),
  fd_fifo_(0) {
}

EvIOLoop::~EvIOLoop() {

}

void EvIOLoop::Init() {
  struct timeval tv;

  ev_base_ = event_base_new();
  tid_ = std::this_thread::get_id();

  Callback<void(evutil_socket_t, short, void*)>::func = std::bind(&EvIOLoop::run_watch_dog,
                                                                  this,
                                                                  std::placeholders::_1,
                                                                  std::placeholders::_2,
                                                                  std::placeholders::_3);

  libevent_callback_t callback_func = static_cast<libevent_callback_t>(Callback<void(evutil_socket_t, short, void*)>::callback);

  event_assign(&watchdog_evt_,
               ev_base_,
               -1,
               EV_PERSIST,
               callback_func,
               (void*)& watchdog_evt_);

  evutil_timerclear(&tv);
  tv.tv_sec = 2;
  event_add(&watchdog_evt_, &tv);

  // init fifi using for manager task
  mode_t mode = 0666;
  std::stringstream ss;
  ss << tid_;
  fifo_name_ = "/tmp/lightingio_" + ss.str();

  unlink(fifo_name_.c_str());
  if (::mkfifo(fifo_name_.c_str(), mode ) < 0) {
    perror ( "failed to mkfifo" );
    exit (1);
  }

	fd_fifo_ = open(fifo_name_.c_str(), O_RDWR | O_NONBLOCK, 0);

	if (fd_fifo_ == -1) {
		perror("open fifo error");
		exit(1);
	}
  libevent_callback_t call_ = [=this](int fd, short fd, void*) {
    this->OnReadFiFo();
  };

  struct event* evfifo = event_new(ev_base_,
                                   fd_fifo_,
                                   EV_WRITE|EV_READ|EV_PERSIST,
                                   call_,
                                   event_self_cbarg());

	event_add(evfifo, NULL);

  std::cout << "successfully create FIFO in path:" << fifo_name_ << std::endl;

  StartTimer(std::bind(&EvIOLoop::test, this));
  int i = 5;
  StartTimer(std::bind(&EvIOLoop::test2, this, i));
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

int EvIOLoop::StartTimer(const TimerFunctor& funtor) {
  timer_functor_.push_back(funtor);
  return 100;
}

void EvIOLoop::run_watch_dog(int sock, short event, void *arg) {
  std::cout << __FUNCTION__ << " sock:"
            << sock << "event:" <<  event << std::endl;
}

}
