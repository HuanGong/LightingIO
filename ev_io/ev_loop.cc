
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

struct LoopContext {
  explicit LoopContext() : loop(NULL), is_active(false) {}
  void InitLoop(EventLoop* el) {
    if (!is_active) {
      loop = el;
      is_active = true;
    }
  }
  EventLoop* loop;
  bool is_active;
  // Holds a list of events pending timers for cleanup when the loop exits.
  std::list<TimerEvent*> pending_timers_;
};

LoopContext& CurrentContext() {
  static thread_local LoopContext loop_context;
  return loop_context;
}

EventLoop::EventLoop(std::string name)
  : event_base_(event_base_new()),
    wakeup_pipe_in_(0),
    wakeup_pipe_out_(0),
    wakeup_event_(new event()) {
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

bool EventLoop::IsCurrent() const {
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
  LoopContext& context = CurrentContext();
  context.InitLoop(this);

  tid_ = std::this_thread::get_id();

  while (context.is_active) {
    event_base_loop(event_base_, 0);
  }

  //pthread_setspecific(GetQueuePtrTls(), nullptr);
  for (TimerEvent* timer : context.pending_timers_) {
    delete timer;
  }
  context.loop = NULL;
}

void EventLoop::PostTask(std::unique_ptr<QueuedTask> task) {
  DCHECK(task.get());
  if (IsCurrent()) {
    if (event_base_once(event_base_, -1, EV_TIMEOUT, &EventLoop::RunTask,
                        task.get(), nullptr) == 0) {
      task.release();
    }
  } else {
    QueuedTask* task_id = task.get();  // Only used for comparison.
    {
      //CritScope lock(&pending_lock_);
      std::unique_lock<std::mutex>  lck(pending_lock_);
      pending_.push_back(std::move(task));
    }
    char message = kRunTask;
    if (write(wakeup_pipe_in_, &message, sizeof(message)) != sizeof(message)) {
      //LOG(WARNING) << "Failed to queue task.";
      std::cout << "Failed to queue task.";
      std::unique_lock<std::mutex>  lck(pending_lock_);
      //CritScope lock(&pending_lock_);
      pending_.remove_if([task_id](std::unique_ptr<QueuedTask>& t) {
        return t.get() == task_id;
      });
    }
  }
}


//static libevent callback_t
void EventLoop::RunTask(int fd, short flags, void* context) {
  auto* task = static_cast<QueuedTask*>(context);
  if (task->Run()) {
    delete task;
  }
}


//static
void EventLoop::OnWakeup(int socket, short flags, void* context) {
  LoopContext& ctx = CurrentContext();
  DCHECK(ctx.loop == context);
  DCHECK(ctx.loop->wakeup_pipe_out_ == socket);
  char buf;
  DCHECK(sizeof(buf) == read(socket, &buf, sizeof(buf)));
  switch (buf) {
    case kQuit:
      ctx.is_active = false;
      event_base_loopbreak(ctx.loop->event_base_);
      break;
    case kRunTask: {
      std::unique_ptr<QueuedTask> task;
      {
        std::unique_lock<std::mutex> lck(ctx.loop->pending_lock_);
        //CritScope lock(&ctx->queue->pending_lock_);
        DCHECK(!ctx.loop->pending_.empty());
        task = std::move(ctx.loop->pending_.front());
        ctx.loop->pending_.pop_front();
        DCHECK(task.get());
      }
      if (!task->Run()) {
        task.release();
      }
      break;
    }
    /*
    case kRunReplyTask: {
      scoped_refptr<ReplyTaskOwnerRef> reply_task;
      {
        CritScope lock(&ctx->queue->pending_lock_);
        for (auto it = ctx->queue->pending_replies_.begin();
             it != ctx->queue->pending_replies_.end(); ++it) {
          if ((*it)->HasOneRef()) {
            reply_task = std::move(*it);
            ctx->queue->pending_replies_.erase(it);
            break;
          }
        }
      }
      reply_task->Run();
      break;
    } */
    default:
      std::cout << __FUNCTION__ << " should not readched !!!!" << std::endl;
      break;
  }
}


EventLoop* EventLoop::Current() {
  return CurrentContext().loop;
}

}//namespace

