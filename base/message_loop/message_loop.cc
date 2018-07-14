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

#include <iostream>
#include "file_util_linux.h"
#include "base/time/time_utils.h"

namespace base {

static const char kQuit = 1;
static const char kRunTask = 2;
static const char kRunReplyTask = 3;

static thread_local MessageLoop2* threadlocal_current_ = NULL;
//static
MessageLoop2* MessageLoop2::Current() {
  return threadlocal_current_;
}

//   signal(SIGPIPE, SIG_IGN);
void IgnoreSigPipeSignalOnCurrentThread2() {
  sigset_t sigpipe_mask;
  sigemptyset(&sigpipe_mask);
  sigaddset(&sigpipe_mask, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sigpipe_mask, nullptr);
}

class MessageLoop2::ReplyTaskOwner {
public:
  ReplyTaskOwner(std::unique_ptr<QueuedTask> reply)
    : reply_(std::move(reply)) {
  }
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

class MessageLoop2::SetTimerTask : public QueuedTask {
 public:
  SetTimerTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds)
    : task_(std::move(task)),
      milliseconds_(milliseconds),
      posted_(time_ms()) {
  }
 private:
  bool Run() override {
    uint32_t post_time = time_ms() - posted_;
    uint32_t new_time = post_time > milliseconds_ ? 0 : milliseconds_ - post_time;
    CHECK(MessageLoop2::Current() != NULL);
    if (new_time == 0) {
      LOG(INFO) <<  "Direct RUN Delay Task";
      task_->Run();
      task_.release();
    } else {
      LOG(INFO) <<  "Post Delay Again:" << new_time << " ms";
      MessageLoop2::Current()->PostDelayTask(std::move(task_), new_time);
    }
    return true;
  }

  std::unique_ptr<QueuedTask> task_;
  const uint32_t milliseconds_;
  const uint32_t posted_;
};

class MessageLoop2::PostAndReplyTask : public QueuedTask {
public:
  PostAndReplyTask(std::unique_ptr<QueuedTask> task,
                   std::unique_ptr<QueuedTask> reply,
                   MessageLoop2* run_reply_loop,
                   int reply_wakeup_pipe)
    : task_(std::move(task)),
      reply_wakeup_pipe_(reply_wakeup_pipe),
      reply_task_owner_(new RefCountedObject<ReplyTaskOwner>(std::move(reply))) {
      run_reply_loop->PrepareReplyTask(reply_task_owner_);
  }

  ~PostAndReplyTask() override {
    reply_task_owner_ = nullptr;
    IgnoreSigPipeSignalOnCurrentThread2();

    char message = kRunReplyTask;
    /*triggle to run this reply on run_reply_loop*/
    int num = write(reply_wakeup_pipe_, &message, sizeof(message));
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
  int reply_wakeup_pipe_; //owner by MessageLoop2 who run this replytask;
  scoped_refptr<RefCountedObject<ReplyTaskOwner>> reply_task_owner_;
};

MessageLoop2::MessageLoop2()
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

  wakeup_event_ = FdEvent::create(wakeup_pipe_out_, EPOLLIN);
  CHECK(wakeup_event_);

  wakeup_event_->SetReadCallback(std::bind(&MessageLoop2::OnWakeup, this));

  running_.clear();
}

MessageLoop2::~MessageLoop2() {

  CHECK(!IsInLoopThread());

  LOG(ERROR) << " MessageLoop2::~MessageLoop2 Gone " << loop_name_;
  struct timespec ts;
  char message = kQuit;
  while (write(wakeup_pipe_in_, &message, sizeof(message)) != sizeof(message)) {
    // The queue is full, so we have no choice but to wait and retry.
    LOG_IF(ERROR, EAGAIN == errno) << "Write to pipe Failed:" << errno;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;
    nanosleep(&ts, nullptr);
  }

  WaitLoopEnd();
  status_.store(ST_STOPED);

  wakeup_event_.reset();

  IgnoreSigPipeSignalOnCurrentThread2();

  close(wakeup_pipe_in_);
  close(wakeup_pipe_out_);

  wakeup_pipe_in_ = -1;
  wakeup_pipe_out_ = -1;

  event_pump_.reset();
}

void MessageLoop2::SetLoopName(std::string name) {
  loop_name_ = name;
}

bool MessageLoop2::IsInLoopThread() const {
  return event_pump_ && event_pump_->IsInLoopThread();
}

void MessageLoop2::Start() {
  bool run = running_.test_and_set(std::memory_order_acquire);
  if (run == true) {
    LOG(ERROR) << " Messageloop Can't Start Twice";
    return;
  }

  thread_ptr_.reset(new std::thread(std::bind(&MessageLoop2::ThreadMain, this)));

  do {
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  } while(status_ != ST_STARTED);
}

void MessageLoop2::WaitLoopEnd() {
  int32_t expect = 0, replace = 1;
  bool has_set = has_join_.compare_exchange_strong(expect, replace);
  if (has_set == false) {
    LOG(INFO) << "Thread Has Join by Others";
    return;
  }

  if (thread_ptr_ && thread_ptr_->joinable()) {
    thread_ptr_->join();
  }
}

void MessageLoop2::BeforePumpRun() {
}

void MessageLoop2::AfterPumpRun() {
}

void MessageLoop2::ThreadMain() {
  threadlocal_current_ = this;
  event_pump_->SetLoopThreadId(std::this_thread::get_id());

  status_.store(ST_STARTED);
  event_pump_->InstallFdEvent(wakeup_event_.get());

  //delegate_->BeforeLoopRun();
  LOG(INFO) << "MessageLoop: " << loop_name_ << " Start Runing";

  event_pump_->Run();

  //delegate_->AfterLoopRun();
  LOG(INFO) << "MessageLoop: " << loop_name_ << " Stop Runing";
  event_pump_->RemoveFdEvent(wakeup_event_.get());
  status_.store(ST_STOPED);
  threadlocal_current_ = NULL;
}

void MessageLoop2::PostDelayTask(std::unique_ptr<QueuedTask> task, uint32_t ms) {
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

void MessageLoop2::PostTask(std::unique_ptr<QueuedTask> task) {
  if (status_ != ST_STARTED) {
    LOG(ERROR) << "MessageLoop: " << loop_name_ << " Not Started";
    return;
  }

  CHECK(task.get());
  if (IsInLoopThread()) {
    QueuedTask* task_id = task.get();  // Only used for comparison.

    {
      std::unique_lock<std::mutex>  lck(pending_lock_);
      pending_.push_back(std::move(task));
    }

    char message = kRunTask;
    if (write(wakeup_pipe_in_, &message, sizeof(message)) != sizeof(message)) {
      LOG(ERROR) << "Failed to schedule this task.";
      CHECK(task_id == pending_.back().get());
      pending_.pop_back();
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
void MessageLoop2::OnWakeup() {
  DCHECK(wakeup_event_->fd() == wakeup_pipe_out_);

  char buf;
  CHECK(sizeof(buf) == read(wakeup_pipe_out_, &buf, sizeof(buf)));
  switch (buf) {
    case kQuit:
      event_pump_->Quit();
      break;
    case kRunTask: {
      std::unique_ptr<QueuedTask> task;
      DCHECK(!pending_.empty());
      if (!pending_.empty()) {
        std::unique_lock<std::mutex> lck(pending_lock_);
        task = std::move(pending_.front());
        pending_.pop_front();
        DCHECK(task.get());
      }
      if (task && !task->Run()) {
        task.release();
      }
      break;
    }
    case kRunReplyTask: {
      scoped_refptr<ReplyTaskOwnerRef> reply_task;
      {
        std::unique_lock<std::mutex> lck(pending_lock_);
        for (auto iter = pending_replies_.begin();
             iter != pending_replies_.end(); iter++) {
          if ((*iter)->HasOneRef()) {
            reply_task = std::move(*iter);
            pending_replies_.erase(iter);
            break;
          }
        }
      }
      if (reply_task.get())
        reply_task->Run();
    }
    break;
    default:
      LOG(ERROR) << "Shout Not Reached HERE";
      break;
  }
}

void MessageLoop2::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                   std::unique_ptr<QueuedTask> reply,
                                   MessageLoop2* reply_loop) {
  CHECK(reply_loop);

  std::unique_ptr<QueuedTask> wrapper(new PostAndReplyTask(std::move(task),
                                                           std::move(reply),
                                                           reply_loop,
                                                           reply_loop->wakeup_pipe_in_));
  PostTask(std::move(wrapper));
}

bool MessageLoop2::PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                                   std::unique_ptr<QueuedTask> reply) {
  if (NULL == Current()) {
    LOG(ERROR) << "The Reply loop is NULL!";
    return false;
  }
  PostTaskAndReply(std::move(task), std::move(reply), Current());
  return true;
}

void MessageLoop2::PrepareReplyTask(scoped_refptr<ReplyTaskOwnerRef> reply_task) {
  DCHECK(reply_task);
  std::unique_lock<std::mutex> lck(pending_lock_);
  pending_replies_.push_back(std::move(reply_task));
}

bool MessageLoop2::InstallSigHandler(int sig, const SigHandler handler) {
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

void MessageLoop2::QuitLoop() {
  event_pump_->Quit();
}

};
