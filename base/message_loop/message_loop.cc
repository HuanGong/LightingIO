#include "message_loop.h"

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
#include <limits.h>
#include <string>
#include <sstream>
#include <functional>
#include <event2/util.h>

#include <base/utils/hpp_2_c_helper.h>

#include <iostream>

namespace base {

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

class MessageLoop::ReplyTaskOwner {
  public:
    ReplyTaskOwner(std::unique_ptr<QueuedTask> reply)
      : reply_(std::move(reply)) {}
    void Run() {
      DCHECK(reply_);
      if (run_task_ && (!reply_->Run())) {
        reply_.release();
      }
      reply_.reset();
    }
    void set_should_run_task() {
      DCHECK(!run_task_);
      run_task_ = true;
    }
  private:
    std::unique_ptr<QueuedTask> reply_;
    bool run_task_ = false;
};

struct TimerEvent {
  explicit TimerEvent(std::unique_ptr<QueuedTask> task)
      : task(std::move(task)) {}
  ~TimerEvent() { event_del(&ev); }
  event ev;
  std::unique_ptr<QueuedTask> task;
};

class MessageLoop::SetTimerTask : public QueuedTask {
 public:
  SetTimerTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds)
      : task_(std::move(task)),
        milliseconds_(milliseconds),
        posted_(time_ms()) {}
 private:
  bool Run() override {
    uint32_t post_time = time_ms() - posted_;
    MessageLoop::Current()->PostDelayTask(std::move(task_),
                                            post_time > milliseconds_ ? 0 : milliseconds_ - post_time);
    return true;
  }

  std::unique_ptr<QueuedTask> task_;
  const uint32_t milliseconds_;
  const uint32_t posted_;
};

class MessageLoop::PostAndReplyTask : public QueuedTask {
  public:
    PostAndReplyTask(std::unique_ptr<QueuedTask> task,
                     std::unique_ptr<QueuedTask> reply,
                     MessageLoop* reply_queue,
                     int reply_pipe)
      : task_(std::move(task)),
        reply_pipe_(reply_pipe),
        reply_task_owner_(new RefCountedObject<ReplyTaskOwner>(std::move(reply))) {
        reply_queue->PrepareReplyTask(reply_task_owner_);
      }

    ~PostAndReplyTask() override {
      reply_task_owner_ = nullptr;
      IgnoreSigPipeSignalOnCurrentThread();
      // Send a signal to the reply queue that the reply task can run now.
      // Depending on whether |set_should_run_task()| was called by the
      // PostAndReplyTask(), the reply task may or may not actually run.
      // In either case, it will be deleted.
      char message = kRunReplyTask;
      int num = write(reply_pipe_, &message, sizeof(message));
      CHECK(num == sizeof(message));
    }
  private:
    bool Run() override {
      if (!task_->Run()) {
        task_.release();
      }
      reply_task_owner_->set_should_run_task();
      return true;
    }

    std::unique_ptr<QueuedTask> task_;
    int reply_pipe_;
    scoped_refptr<RefCountedObject<ReplyTaskOwner>> reply_task_owner_;
};

bool SetNonBlocking(int fd) {
  const int flags = fcntl(fd, F_GETFL);
  CHECK(flags != -1);
  return (flags & O_NONBLOCK) || fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void EventAssign(struct event* ev,
                 struct event_base* base,
                 int fd,
                 short events,
                 void (*callback)(int, short, void*),
                 void* arg) {
  LOG_IF(ERROR, (0 != event_assign(ev, base, fd, events, callback, arg)))
    << "libevent event_assign failed";
}

struct LoopContext {
  explicit LoopContext() : loop(NULL), is_active(false) {}
  void InitLoop(MessageLoop* el) {
    if (!is_active) {
      loop = el;
      is_active = true;
    }
  }
  MessageLoop* loop;
  bool is_active;
  // Holds a list of events pending timers for cleanup when the loop exits.
  std::list<TimerEvent*> pending_timers_;
};

LoopContext& CurrentContext() {
  static thread_local LoopContext loop_context;
  return loop_context;
}

MessageLoop::MessageLoop(std::string name)
  : event_base_(event_base_new()),
    wakeup_pipe_in_(0),
    wakeup_pipe_out_(0),
    wakeup_event_(new event()) {
  loop_name_ = name;
}

MessageLoop::~MessageLoop() {

  DCHECK(!IsInLoopThread());

  struct timespec ts;
  char message = kQuit;
  while (write(wakeup_pipe_in_, &message, sizeof(message)) != sizeof(message)) {
    // The queue is full, so we have no choice but to wait and retry.
    LOG_IF(ERROR, EAGAIN == errno) << "Write to pipe Failed:" << errno;
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

bool MessageLoop::IsInLoopThread() const {
  return tid_ == std::this_thread::get_id();
}

void MessageLoop::Start() {

  int fds[2];
  CHECK(pipe(fds) == 0);

  SetNonBlocking(fds[0]);
  SetNonBlocking(fds[1]);

  wakeup_pipe_out_ = fds[0];
  wakeup_pipe_in_ = fds[1];

  EventAssign(wakeup_event_.get(),
              event_base_,
              wakeup_pipe_out_,
              EV_READ | EV_PERSIST, OnWakeup, this);
  event_add(wakeup_event_.get(), 0);

  thread_ptr_.reset(new std::thread(std::bind(&MessageLoop::ThreadMain, this)));
}

void MessageLoop::ThreadMain() {
  LoopContext& context = CurrentContext();
  context.InitLoop(this);

  tid_ = std::this_thread::get_id();

  LOG(INFO) << "MessageLoop: " << loop_name_ << " Start Runing";
  while (context.is_active) {
    event_base_loop(event_base_, 0);
  }
  //pthread_setspecific(GetQueuePtrTls(), nullptr);
  for (TimerEvent* timer : context.pending_timers_) {
    delete timer;
  }
  context.loop = NULL;
}

void MessageLoop::PostDelayTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds) {

  LoopContext& ctx = CurrentContext();

  if (IsInLoopThread()) {
    TimerEvent* timer = new TimerEvent(std::move(task));
    EventAssign(&timer->ev, event_base_, -1, 0, &MessageLoop::RunTimer, timer);
    ctx.pending_timers_.push_back(timer);
    timeval tv = {milliseconds / 1000, (milliseconds % 1000) * 1000};
    event_add(&timer->ev, &tv);
  } else {
    PostTask(std::unique_ptr<QueuedTask>(new SetTimerTask(std::move(task), milliseconds)));
  }
  return;
}

void MessageLoop::PostTask(std::unique_ptr<QueuedTask> task) {
  CHECK(task.get());
  if (IsInLoopThread()) {
    DLOG(INFO) << "schedule a task on loop thread";
    if (event_base_once(event_base_,
                        -1,
                        EV_TIMEOUT,
                        &MessageLoop::RunTask,
                        task.get(), nullptr) == 0) {
      task.release();
    }
  } else {
    QueuedTask* task_id = task.get();  // Only used for comparison.
    {
      std::unique_lock<std::mutex>  lck(pending_lock_);
      pending_.push_back(std::move(task));
    }
    char message = kRunTask;
    if (write(wakeup_pipe_in_, &message, sizeof(message)) != sizeof(message)) {
      LOG(ERROR) << "Failed to schedule this task.";
      std::unique_lock<std::mutex>  lck(pending_lock_);
      pending_.remove_if([task_id](std::unique_ptr<QueuedTask>& t) {
        return t.get() == task_id;
      });
    }
  }
}

//static
void MessageLoop::RunTimer(int fd, short flags, void* context) {
  TimerEvent* timer = static_cast<TimerEvent*>(context);
  if (!timer->task->Run()) {
    timer->task.release();
  }
  auto& ctx = CurrentContext();
  ctx.pending_timers_.remove(timer);
  delete timer;
}

//static libevent callback_t
void MessageLoop::RunTask(int fd, short flags, void* context) {
  auto* task = static_cast<QueuedTask*>(context);
  if (task->Run()) {
    delete task;
  }
}


//static
void MessageLoop::OnWakeup(int socket, short flags, void* context) {
  LoopContext& ctx = CurrentContext();
  DCHECK(ctx.loop == context);
  DCHECK(ctx.loop->wakeup_pipe_out_ == socket);
  char buf;
  CHECK(sizeof(buf) == read(socket, &buf, sizeof(buf)));
  switch (buf) {
    case kQuit:
      ctx.is_active = false;
      event_base_loopbreak(ctx.loop->event_base_);
      break;
    case kRunTask: {
      std::unique_ptr<QueuedTask> task;
      DCHECK(!ctx.loop->pending_.empty());
      {
        std::unique_lock<std::mutex> lck(ctx.loop->pending_lock_);
        task = std::move(ctx.loop->pending_.front());
        ctx.loop->pending_.pop_front();
      }
      DCHECK(task.get());
      if (!task->Run()) {
        task.release();
      }
      break;
    }
    case kRunReplyTask: {
      scoped_refptr<ReplyTaskOwnerRef> reply_task;
      {
        std::unique_lock<std::mutex> lck(ctx.loop->pending_lock_);
        for (auto it = ctx.loop->pending_replies_.begin();
             it != ctx.loop->pending_replies_.end(); ++it) {
          if ((*it)->HasOneRef()) {
            reply_task = std::move(*it);
            ctx.loop->pending_replies_.erase(it);
            break;
          }
        }
      }
      reply_task->Run();
    }
    break;
    default:
      LOG(ERROR) << "Shout Not Reached HERE";
      break;
  }
}

void MessageLoop::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                   std::unique_ptr<QueuedTask> reply,
                                   MessageLoop* reply_queue) {
  std::unique_ptr<QueuedTask> wrapper_task(
    new PostAndReplyTask(std::move(task),
                         std::move(reply),
                         reply_queue,
                         reply_queue->wakeup_pipe_in_));

  PostTask(std::move(wrapper_task));
}

bool MessageLoop::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                   std::unique_ptr<QueuedTask> reply) {
  if (!Current()) {
    LOG(ERROR) << "This Thread not attach to any MessageLoop, please check it!";
    return false;
  }
  PostTaskAndReply(std::move(task), std::move(reply), Current());
  return true;
}

void MessageLoop::PrepareReplyTask(scoped_refptr<ReplyTaskOwnerRef> reply_task) {
  DCHECK(reply_task);
  std::unique_lock<std::mutex> lck(pending_lock_);
  pending_replies_.push_back(std::move(reply_task));
}

//static
MessageLoop* MessageLoop::Current() {
  return CurrentContext().loop;
}


}//namespace

