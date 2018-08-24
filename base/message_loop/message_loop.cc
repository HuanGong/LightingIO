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
#include "base/time/time_utils.h"

#include "base/coroutine/coroutine_runner.h"

namespace base {

static const char kQuit = 1;
static const char kRunReplyTask = 3;

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

/* a helper class for re-schedule timer task */
class MessageLoop::SetTimerTask : public QueuedTask {
 public:
  SetTimerTask(std::unique_ptr<QueuedTask> task, uint32_t ms)
    : task_(std::move(task)),
      milliseconds_(ms),
      posted_(time_ms()) {
  }
 private:
  bool Run() override {
    uint32_t post_time = time_ms() - posted_;
    uint32_t new_time = post_time > milliseconds_ ? 0 : milliseconds_ - post_time;
    CHECK(MessageLoop::Current() != NULL);
    if (new_time == 0) {
      VLOG(GLOG_VINFO) <<  "Time was expired, run this timer task directly";
      task_->Run(); task_.release();
    } else {
      VLOG(GLOG_VINFO) <<  "Re-Schedule timer in loop, will run task after " << new_time << " ms";
      MessageLoop::Current()->PostDelayTask(std::move(task_), new_time);
    }
    return true;
  }
  std::unique_ptr<QueuedTask> task_;
  const uint32_t milliseconds_;
  const uint32_t posted_;
};

class MessageLoop::BindReplyTask : public QueuedTask {
public:
  static std::unique_ptr<QueuedTask> Create(std::unique_ptr<QueuedTask>& task,
                                            std::unique_ptr<QueuedTask>& reply,
                                            MessageLoop* reply_loop,
                                            int notify_fd) {
    return std::unique_ptr<QueuedTask>(new BindReplyTask(task, reply, reply_loop, notify_fd));
  }
  bool Run() override {
    static uint64_t msg = 1;

    if (!task_->Run()) {
      task_.release(); // remove it
    }
    holder_->CommitReply();
    int ret = ::write(notify_fd_, &msg, sizeof(msg));
    CHECK(ret == sizeof(msg));
    return true;
  }
private:
  BindReplyTask(std::unique_ptr<QueuedTask>& task,
                std::unique_ptr<QueuedTask>& reply,
                MessageLoop* reply_loop,
                int notify_fd) :
    notify_fd_(notify_fd),
    task_(std::move(task)) {

    CHECK(reply_loop);
    holder_ = ReplyHolder::Create(std::move(reply));
    reply_loop->ScheduleFutureReply(holder_);
  }

  int notify_fd_;
  std::unique_ptr<QueuedTask> task_;
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

  wakeup_event_ = FdEvent::Create(wakeup_pipe_out_, EPOLLIN);
  CHECK(wakeup_event_);
  wakeup_event_->SetReadCallback(std::bind(&MessageLoop::OnHandleCommand, this));

  task_event_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  task_event_ = FdEvent::Create(task_event_fd_, EPOLLIN);
  CHECK(task_event_);
  task_event_->SetReadCallback(std::bind(&MessageLoop::RunScheduledTask, this, ScheduledTaskType::TaskTypeDefault));

  reply_event_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  reply_event_ = FdEvent::Create(reply_event_fd_, EPOLLIN);
  CHECK(reply_event_);
  reply_event_->SetReadCallback(std::bind(&MessageLoop::RunScheduledTask, this, ScheduledTaskType::TaskTypeReply));

  //coroutine task
  coro_task_event_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  coro_task_event_ = FdEvent::Create(coro_task_event_fd_, EPOLLIN);
  coro_task_event_->SetReadCallback(std::bind(&MessageLoop::RunCoroutineTask, this, true));
  CHECK(coro_task_event_);

  running_.clear();
}

MessageLoop::~MessageLoop() {

  CHECK(!IsInLoopThread());

  LOG(INFO) << " MessageLoop " << LoopName() << " Gone...";
  while (write(wakeup_pipe_in_, &kQuit, sizeof(kQuit)) != sizeof(kQuit)) {
    LOG_IF(ERROR, EAGAIN == errno) << "Write to pipe Failed:" << errno;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }

  WaitLoopEnd();

  wakeup_event_.reset();
  IgnoreSigPipeSignalOnCurrentThread2();
  close(wakeup_pipe_in_);
  //close(wakeup_pipe_out_);

  task_event_.reset();
  reply_event_.reset();
  coro_task_event_.reset();
  task_event_fd_ = -1;
  reply_event_fd_ = -1;
  wakeup_pipe_in_ = -1;
  wakeup_pipe_out_ = -1;
  coro_task_event_fd_ = -1;

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
  LOG_IF(INFO, run) << "Messageloop " << LoopName() << " aready runing...";
  if (run) {return;}

  thread_ptr_.reset(new std::thread(std::bind(&MessageLoop::ThreadMain, this)));

  do {
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  } while(status_ != ST_STARTED);
}

void MessageLoop::WaitLoopEnd(int32_t ms) {
  int32_t has_join = has_join_.exchange(1);
  if (has_join == 0 && thread_ptr_ && thread_ptr_->joinable()) {
    thread_ptr_->join();
  }

  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  } while(status_ == ST_STARTED);
}

void MessageLoop::BeforePumpRun() {
}

void MessageLoop::AfterPumpRun() {
}

void MessageLoop::ThreadMain() {
  threadlocal_current_ = this;
  event_pump_->SetLoopThreadId(std::this_thread::get_id());

  status_.store(ST_STARTED);
  event_pump_->InstallFdEvent(wakeup_event_.get());
  event_pump_->InstallFdEvent(task_event_.get());
  event_pump_->InstallFdEvent(reply_event_.get());
  event_pump_->InstallFdEvent(coro_task_event_.get());

  //delegate_->BeforeLoopRun();
  LOG(INFO) << "MessageLoop: " << loop_name_ << " Start Runing";

  event_pump_->Run();

  //delegate_->AfterLoopRun();
  event_pump_->RemoveFdEvent(task_event_.get());
  event_pump_->RemoveFdEvent(reply_event_.get());
  event_pump_->RemoveFdEvent(wakeup_event_.get());
  event_pump_->RemoveFdEvent(coro_task_event_.get());

  LOG(INFO) << "MessageLoop: " << loop_name_ << " Stop Runing";
  status_.store(ST_STOPED);
  threadlocal_current_ = NULL;
}

void MessageLoop::PostDelayTask(std::unique_ptr<QueuedTask> task, uint32_t ms) {
  if (status_ != ST_STARTED) {
    LOG(ERROR) << "MessageLoop: " << loop_name_ << " Not Started";
    return;
  }

  if (IsInLoopThread()) {
    RefTimerEvent timer = TimerEvent::CreateOneShotTimer(ms, std::move(task));
    event_pump_->ScheduleTimer(timer);
  } else {
    PostTask(std::unique_ptr<QueuedTask>(new SetTimerTask(std::move(task), ms)));
  }
}

bool MessageLoop::PostTask(std::unique_ptr<QueuedTask> task) {
  const static uint64_t count = 1;

  if (status_ != ST_STARTED) {
    LOG(ERROR) << "MessageLoop: " << loop_name_ << " Not Started";
    return false;
  }

  //nested task handle; alway true for this case; why?
  //bz waitIO timeout can also will triggle this
  if (IsInLoopThread()) {
    inloop_scheduled_task_.push_back(std::move(task));
    if (write(task_event_fd_, &count, sizeof(count)) < 0) {
      LOG(ERROR) << "write to eventfd failed, errno:" << errno;
    }
    return true;
  }

  QueuedTask* task_id = task.get();  // Only used for comparison.
  CHECK(task_id);

  {
    base::SpinLockGuard guard(task_lock_);
    scheduled_task_.push_back(std::move(task));
  }

  if (write(task_event_fd_, &count, sizeof(count)) < 0) {
    LOG(ERROR) << "write to eventfd failed, errno:" << errno;
    {
      base::SpinLockGuard guard(task_lock_);
      scheduled_task_.remove_if([task_id](std::unique_ptr<QueuedTask>& t) {
        return t.get() == task_id;
      });
    }
    return false;
  }
  return true;
}

void MessageLoop::RunNestedTask() {
  CHECK(IsInLoopThread());

  for (auto& task : inloop_scheduled_task_) {
    if (task && !task->Run()) {
      task.release();
    }
  }
  inloop_scheduled_task_.clear();
}

void MessageLoop::RunScheduledTask(ScheduledTaskType type) {

  switch(type) {
    case ScheduledTaskType::TaskTypeDefault: {
      uint64_t count = 0;
      int ret = read(task_event_fd_, &count, sizeof(count));
      if (ret < 0) {
        LOG(INFO) << " read return:" << ret << " errno:" << errno;
        return;
      }

      std::list<std::unique_ptr<QueuedTask>> scheduled_tasks;
      {
        base::SpinLockGuard guard(task_lock_);
        scheduled_tasks = std::move(scheduled_task_);
        scheduled_task_.clear();
      }
      for (auto& task : scheduled_tasks) {
        if (task && !task->Run()) {
          task.release();
        }
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
    default:
      LOG(ERROR) << "Shout Not Reached HERE!";
      break;
  }
}

void MessageLoop::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                   std::unique_ptr<QueuedTask> reply,
                                   MessageLoop* reply_loop) {
  CHECK(reply_loop);
  auto wraper = BindReplyTask::Create(task, reply, reply_loop, reply_loop->reply_event_fd_);
  PostTask(std::move(wraper));
}

bool MessageLoop::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                   std::unique_ptr<QueuedTask> reply) {

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

bool MessageLoop::PostCoroTask(StlClosure task) {
  const static uint64_t count = 1;
  if (IsInLoopThread()) {
    inloop_coro_task_.push_back(std::move(task));
    LOG_IF(ERROR, Notify(coro_task_event_fd_, &count, sizeof(count)) < 0) << "InLoop Coro Notify Failed";
    return true;
  }

  {
    base::SpinLockGuard guard(coro_task_lock_);
    coro_task_.push_back(std::move(task));
  }
  int r = Notify(coro_task_event_fd_, &count, sizeof(count));
  if (r < 0) {
    LOG(ERROR) << "Schedule Coro Task Notify Failed";
    /*
       base::SpinLockGuard guard(coro_task_lock_);
       coro_task_.remove_if([task_id](std::unique_ptr<QueuedTask>& t) {
       return t.get() == task_id;
       });
    */
    return false;
  }
  return true;
}

void MessageLoop::RunCoroutineTask(bool with_fd) {
  uint64_t count = 0;
  if (with_fd) {
    int ret = read(coro_task_event_fd_, &count, sizeof(count));
    LOG_IF(ERROR, ret < 0) << "read corotask fd fail:" << ret << " errno:" << errno;
  }

  std::list<base::StlClosure> all_coro_task_;
  {
    base::SpinLockGuard guard(coro_task_lock_);
    all_coro_task_ = std::move(coro_task_);
    coro_task_.clear();
  }

  all_coro_task_.splice(all_coro_task_.end(), std::move(inloop_coro_task_));
  inloop_coro_task_.clear();

  count = all_coro_task_.size();
  if (count == 0) {
    return;
  }
  CoroRunner* coro_scheduler = CoroRunner::TlsCurrent();
  coro_scheduler->RunScheduledTasks(std::move(all_coro_task_));
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " Leave, this tick run coro task count:" << count;
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

};
