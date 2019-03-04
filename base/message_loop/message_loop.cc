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
#include <chrono>
#include <map>

#include <sys/eventfd.h>

#include <iostream>
#include "file_util_linux.h"
#include "timeout_event.h"
#include <base/time/time_utils.h>
#include <base/utils/sys_error.h>

namespace base {

static const char kQuit = 1;

//static thread safe
static thread_local MessageLoop* threadlocal_current_ = NULL;
MessageLoop* MessageLoop::Current() {
  return threadlocal_current_;
}

//   signal(SIGPIPE, SIG_IGN);
void IgnoreSigPipeSignalOnCurrentThread2() {
  sigset_t sigpipe_mask;
  sigemptyset(&sigpipe_mask);
  sigaddset(&sigpipe_mask, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sigpipe_mask, nullptr);
}

class TimeoutTaskHelper : public TaskBase {
public:
  TimeoutTaskHelper(ClosurePtr task, EventPump* pump, uint64_t ms)
    : TaskBase(task->TaskLocation()),
      timeout_fn_(std::move(task)),
      event_pump_(pump),
      delay_ms_(ms),
      schedule_time_(time_ms()) {}

  void Run() override {

    uint64_t has_passed_time = time_ms() - schedule_time_;
    int64_t new_delay_ms = delay_ms_ - has_passed_time;
    if (new_delay_ms <= 0) {
      return timeout_fn_->Run();
    }

    VLOG(GLOG_VINFO) <<  "Re-Schedule timer " << new_delay_ms << " ms";

    TimeoutEvent* timeout_ev = TimeoutEvent::CreateOneShotTimer(new_delay_ms, true);

    timeout_ev->InstallTimerHandler(std::move(timeout_fn_));
    event_pump_->AddTimeoutEvent(timeout_ev);
  }
private:
  ClosurePtr timeout_fn_;
  EventPump* event_pump_;
  const uint64_t delay_ms_;
  const uint64_t schedule_time_;
};

class MessageLoop::ReplyTaskHelper : public TaskBase {
public:
  ReplyTaskHelper(ClosurePtr& task, ClosurePtr& reply, MessageLoop* loop, int notify_fd)
    : notify_fd_(notify_fd),
      task_(std::move(task)) {
    CHECK(loop);
    holder_ = ReplyHolder::Create(reply);
    loop->ScheduleFutureReply(holder_);
  }
  void Run() override {
    static uint64_t msg = 1;
    if (task_) {
      task_->Run();
    }
    holder_->CommitReply();
    int ret = ::write(notify_fd_, &msg, sizeof(msg));
    LOG_IF(ERROR, ret != sizeof(msg)) << "run reply failed for task:" << ClosureInfo();
  }
private:
  int notify_fd_;
  ClosurePtr task_;
  std::shared_ptr<ReplyHolder> holder_;
};

MessageLoop::MessageLoop()
  : running_(ATOMIC_FLAG_INIT),
    wakeup_pipe_in_(0),
    wakeup_pipe_out_(0) {

  status_.store(ST_INITING);
  has_join_.store(0);

  event_pump_.reset(new EventPump(this));

  int fds[2];
  bool ret = CreateLocalNoneBlockingPipe(fds);
  CHECK(ret);

  wakeup_pipe_out_ = fds[0];
  wakeup_pipe_in_ = fds[1];

  wakeup_event_ = FdEvent::Create(wakeup_pipe_out_, LtEv::LT_EVENT_READ);
  CHECK(wakeup_event_);
  wakeup_event_->SetReadCallback(std::bind(&MessageLoop::OnHandleCommand, this));

  task_event_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  task_event_ = FdEvent::Create(task_event_fd_, LtEv::LT_EVENT_READ);
  CHECK(task_event_);
  task_event_->SetReadCallback(std::bind(&MessageLoop::RunScheduledTask, this, ScheduledTaskType::TaskTypeDefault));

  reply_event_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  reply_event_ = FdEvent::Create(reply_event_fd_, LtEv::LT_EVENT_READ);
  CHECK(reply_event_);
  reply_event_->SetReadCallback(std::bind(&MessageLoop::RunScheduledTask, this, ScheduledTaskType::TaskTypeReply));

  running_.clear();
}

MessageLoop::~MessageLoop() {
  LOG(INFO) << "MessageLoop: [" << LoopName() << "] Gone...";

  if (status_ == ST_STARTED) {

    while (Notify(wakeup_pipe_in_, &kQuit, sizeof(kQuit)) != sizeof(kQuit)) {
      LOG_IF(ERROR, EAGAIN == errno) << "Write to pipe Failed:" << StrError();
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    WaitLoopEnd();
  }

  wakeup_event_.reset();
  IgnoreSigPipeSignalOnCurrentThread2();
  close(wakeup_pipe_in_);
  //wakeup_pipe_out managed by fdevent
  //close(wakeup_pipe_out_);

  task_event_.reset();
  reply_event_.reset();
  task_event_fd_ = -1;
  reply_event_fd_ = -1;
  wakeup_pipe_in_ = -1;
  wakeup_pipe_out_ = -1;

  event_pump_.reset();
}

void MessageLoop::SetLoopName(std::string name) {
  loop_name_ = name;
}

bool MessageLoop::IsInLoopThread() const {
  return event_pump_->IsInLoopThread();
}

void MessageLoop::Start() {
  bool run = running_.test_and_set(std::memory_order_acquire);
  LOG_IF(INFO, run) << "Messageloop [" << LoopName() << "] aready runing...";
  if (run) {return;}

  thread_ptr_.reset(new std::thread(std::bind(&MessageLoop::ThreadMain, this)));

  {
    std::unique_lock<std::mutex> lk(start_stop_lock_);
    cv_.wait(lk, [&]{return status_ == ST_STARTED;});
  }
}

void MessageLoop::WaitLoopEnd(int32_t ms) {
  CHECK(!IsInLoopThread());

  if (status_ == ST_STOPED) {
    return;
  }

  {
    std::unique_lock<std::mutex> lk(start_stop_lock_);
    cv_.wait(lk, [&]{return status_ == ST_STOPED;});
  }

  int32_t has_join = has_join_.exchange(1);
  if (has_join == 0 && thread_ptr_ && thread_ptr_->joinable()) {
    thread_ptr_->join();
  }
}

void MessageLoop::BeforePumpRun() {
}

void MessageLoop::AfterPumpRun() {
}

void MessageLoop::ThreadMain() {
  threadlocal_current_ = this;
  SetThreadNativeName();
  event_pump_->SetLoopThreadId(std::this_thread::get_id());

  event_pump_->InstallFdEvent(wakeup_event_.get());
  event_pump_->InstallFdEvent(task_event_.get());
  event_pump_->InstallFdEvent(reply_event_.get());

  //delegate_->BeforeLoopRun();
  LOG(INFO) << "MessageLoop: [" << loop_name_ << "] Start Running";

  {
    status_.store(ST_STARTED);
    cv_.notify_all();
  }

  event_pump_->Run();

  //delegate_->AfterLoopRun();
  event_pump_->RemoveFdEvent(task_event_.get());
  event_pump_->RemoveFdEvent(reply_event_.get());
  event_pump_->RemoveFdEvent(wakeup_event_.get());

  threadlocal_current_ = NULL;
  LOG(INFO) << "MessageLoop: [" << loop_name_ << "] Stop Running";

  {
    status_.store(ST_STOPED);
    cv_.notify_all();
  }
}

void MessageLoop::PostDelayTask(std::unique_ptr<TaskBase> task, uint32_t ms) {
  CHECK(status_ == ST_STARTED);

  if (!IsInLoopThread()) {
    PostTask(ClosurePtr(new TimeoutTaskHelper(std::move(task), event_pump_.get(), ms)));
    return;
  }
  TimeoutEvent* timeout_ev = TimeoutEvent::CreateOneShotTimer(ms, true);
  timeout_ev->InstallTimerHandler(std::move(task));
  event_pump_->AddTimeoutEvent(timeout_ev);
}

bool MessageLoop::PostTask(std::unique_ptr<TaskBase> task) {
  const static uint64_t count = 1;
  CHECK(status_ == ST_STARTED);

  if (IsInLoopThread()) {
    in_loop_tasks_.push_back(std::move(task));
    LOG_IF(ERROR, Notify(task_event_fd_, &count, sizeof(count)) < 0) << "notify failed:" << StrError();
    return true;
  }

  const Location& loc = task->TaskLocation();
  if (scheduled_tasks_.enqueue(std::move(task))) {
    int ret = Notify(task_event_fd_, &count, sizeof(count));
    LOG_IF(ERROR, ret < 0) << __FUNCTION__ << " task schedule failed:" << StrError() << " task:" << loc.ToString();
    return true;
  }
  return false;
}

void MessageLoop::RunNestedTask() {
  CHECK(IsInLoopThread());
  for (auto& task : in_loop_tasks_) {
    task->Run();
  }
  in_loop_tasks_.clear();
}

void MessageLoop::RunTimerClosure(const TimerEventList& timer_evs) {
  for (auto& te : timer_evs) {
    te->Invoke();
  }
}

void MessageLoop::RunScheduledTask(ScheduledTaskType type) {
  switch(type) {
    case ScheduledTaskType::TaskTypeDefault: {
      uint64_t count = 0;
      int ret = read(task_event_fd_, &count, sizeof(count));
      LOG_IF(INFO, ret < 0) << __FUNCTION__ << " run scheduled task failed:" << StrError(); 

      TaskBasePtr task;  // instead of pinco *p;
      while (scheduled_tasks_.try_dequeue(task)) {
        DCHECK(task);
        task->Run();
      }
    } break;
    case ScheduledTaskType::TaskTypeReply: {

      base::SpinLockGuard guard(future_reply_lock_);
      for (auto iter = future_replys_.begin(); iter != future_replys_.end();) {
        if ((*iter)->IsCommited()) {
          (*iter)->InvokeReply();
          iter = future_replys_.erase(iter);
          continue;
        }
        iter++;
      }
    } break;
    default:
      DCHECK(false);
      LOG(ERROR) << " Should Not Reached Here!!!";
    break;
  }
}

//static
void MessageLoop::OnHandleCommand() {
  DCHECK(wakeup_event_->fd() == wakeup_pipe_out_);

  char buf;
  CHECK(sizeof(buf) == ::read(wakeup_pipe_out_, &buf, sizeof(buf)));
  switch (buf) {
    case kQuit: {
      QuitLoop();
    } break;
    default: {
      DCHECK(false);
      LOG(ERROR) << "Shout Not Reached HERE!";
    } break;
  }
}

void MessageLoop::PostTaskAndReply(std::unique_ptr<TaskBase> task,
                                   std::unique_ptr<TaskBase> reply,
                                   MessageLoop* reply_loop) {
  CHECK(reply_loop);
  PostTask(ClosurePtr(new ReplyTaskHelper(task, reply, reply_loop, reply_loop->reply_event_fd_)));
}

bool MessageLoop::PostTaskAndReply(std::unique_ptr<TaskBase> task,
                                   std::unique_ptr<TaskBase> reply) {

  MessageLoop* reply_loop = Current() ? Current() : this;
  PostTaskAndReply(std::move(task), std::move(reply), reply_loop);
  return true;
}

void MessageLoop::ScheduleFutureReply(std::shared_ptr<ReplyHolder>& reply_holder) {
  CHECK(reply_holder);
  {
    future_reply_lock_.lock();
    future_replys_.push_back(reply_holder);
    future_reply_lock_.unlock();
  }
}

bool MessageLoop::InstallSigHandler(int sig, const SigHandler handler) {
  if (status_ != ST_STARTED) return false;

  if (!IsInLoopThread()) {
    PostTask(NewClosure(std::bind([&](int s, const SigHandler h){
      sig_handlers_.insert(std::make_pair(s, h));
    }, sig, handler)));
  } else {
    sig_handlers_.insert(std::make_pair(sig, handler));
  }
  return true;
}

void MessageLoop::QuitLoop() {
  event_pump_->Quit();
}

int MessageLoop::Notify(int fd, const void* data, size_t count) {
  const static int32_t max_retry_times = 3;
  int ret = 0, retry = 0;

  do {
    ret = ::write(fd, data, count);
    if (ret > 0) {return ret;}

    switch(errno) {
      case EINTR:
      case EAGAIN:
        continue;
      default:
        return ret;
    }
  } while(retry++ < max_retry_times);

  return ret;
}

void MessageLoop::SetThreadNativeName() {
  if (loop_name_.empty()) return;
  LOG(INFO) << __FUNCTION__ << " change message loop name to " << loop_name_;
  pthread_setname_np(pthread_self(), loop_name_.c_str());
  //pthread_setname_np(pthread_self(), s.c_str());
  // set the name (pthread_self() returns the pthread_t of the current thread)
  //pthread_getname_np(pthread_self(), &name[0], sizeof(name));
}

};
