
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
#include <csignal>

#include <event2/util.h>

#include <limits.h>
#include <string>
#include <sstream>
#include <functional>

#include <base/utils/hpp_2_c_helper.h>

#include <iostream>

namespace IO {

static const char kQuit = 1;
static const char kRunTask = 2;
static const char kRunReplyTask = 3;

typedef void (*ev_callback_t)(evutil_socket_t, short, void*);

//   signal(SIGPIPE, SIG_IGN);
void IgnoreSigPipeSignalOnCurrentThread() {
  sigset_t sigpipe_mask;
  sigemptyset(&sigpipe_mask);
  sigaddset(&sigpipe_mask, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sigpipe_mask, nullptr);
}

struct TimerEvent {
  explicit TimerEvent(std::unique_ptr<QueuedTask> task)
      : task(std::move(task)) {}
  ~TimerEvent() { event_del(&ev); }
  event ev;
  std::unique_ptr<QueuedTask> task;
};

bool SetNonBlocking(int fd) {
  const int flags = fcntl(fd, F_GETFL);
  DCHECK(flags != -1);
  return (flags & O_NONBLOCK) || fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void EventAssign(struct event* ev,
                 struct event_base* base,
                 int fd,
                 short events,
                 void (*callback)(int, short, void*),
                 void* arg) {
  if (0 != event_assign(ev, base, fd, events, callback, arg)) {
    std::cout << "event_assign failed" << std::endl;
  }
}

LoopContext& Current() {
  static thread_local LoopContext loop_context;
  return loop_context;
}

EventLoop::EventLoop(std::string name)
  : event_base_(NULL),
    wakeup_pipe_in_(0),
    wakeup_pipe_out_(0) {
  loop_name_ = name;
}

EventLoop::~EventLoop() {

  DCHECK(!IsCurrent());

  struct timespec ts;
  char message = kQuit;
  while (write(wakeup_pipe_in_, &message, sizeof(message)) != sizeof(message)) {
    // The queue is full, so we have no choice but to wait and retry.
    if (EAGAIN == errno) {
      std::cout << "EAGAIN == errno when write quit message" << std::endl;
    }
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;
    nanosleep(&ts, nullptr);
  }

  if (thread_ptr_ && thread_ptr_->joinable()) {
    thread_ptr_->join();
  }

  event_del(wakeup_event_.get());

  IgnoreSigPipeSignalOnCurrentThread();

  close(wakeup_pipe_in_);
  close(wakeup_pipe_out_);

  wakeup_pipe_in_ = -1;
  wakeup_pipe_out_ = -1;

  event_base_free(event_base_);
}

bool EventLoop::IsCurrent() {
  return tid_ == std::this_thread::get_id();
}

void EventLoop::Start() {

  int fds[2];
  DCHECK(pipe(fds) == 0);

  SetNonBlocking(fds[0]);
  SetNonBlocking(fds[1]);

  wakeup_pipe_out_ = fds[0];
  wakeup_pipe_in_ = fds[1];

  EventAssign(wakeup_event_.get(),
              event_base_,
              wakeup_pipe_out_,
              EV_READ | EV_PERSIST, OnWakeup, this);
  event_add(wakeup_event_.get(), 0);

  thread_ptr_.reset(new std::thread(std::bind(&EventLoop::LoopMain, this)));
}

void EventLoop::LoopMain() {
  //TaskQueue* me = static_cast<TaskQueue*>(context);
/*
  QueueContext queue_context(me);
  pthread_setspecific(GetQueuePtrTls(), &queue_context);

  while (queue_context.is_active)
    event_base_loop(me->event_base_, 0);

  pthread_setspecific(GetQueuePtrTls(), nullptr);

  for (TimerEvent* timer : queue_context.pending_timers_)
    delete timer;
*/
}

//static
void EventLoop::OnWakeup(int socket, short flags, void* context) {
  std::cout << "EventLoop::OnWakeup ......" << std::endl;
}


EventLoop& EventLoop::Current() {

}

}//namespace
