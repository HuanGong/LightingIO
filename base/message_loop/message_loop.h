#ifndef _BASE_MSG_EVENT_LOOP_H_H
#define _BASE_MSG_EVENT_LOOP_H_H

#include <atomic>
#include <thread>
#include <queue>
#include <mutex>

#include "fd_event.h"
#include "event_pump.h"
#include "glog/logging.h"
#include "base_constants.h"

#include "closure/task_queue.h"
#include "memory/spin_lock.h"
#include "memory/scoped_ref_ptr.h"
#include "memory/refcountedobject.h"

#include <condition_variable>

namespace base {

enum LoopState {
  ST_INITING = 1,
  ST_STARTED = 2,
  ST_STOPED  = 3
};

class ReplyHolder {
public:
  static std::shared_ptr<ReplyHolder> Create(TaskBasePtr& task) {
    return std::make_shared<ReplyHolder>(std::move(task));
  }
  ReplyHolder(TaskBasePtr task) : commited_(false), reply_(std::move(task)) {}
  bool InvokeReply() {
    if (!reply_ || !commited_) {
      return false;
    }
    reply_->Run();
    return true;
  }
  inline void CommitReply() {commited_ = true;}
  inline bool IsCommited() {return commited_;};
private:
  bool commited_;
  TaskBasePtr reply_;
};

class MessageLoop : public PumpDelegate {
  public:
    static MessageLoop* Current();

    typedef enum {
      TaskTypeDefault = 0,
      TaskTypeReply   = 1,
      TaskTypeCtrl    = 2
    } ScheduledTaskType;


    MessageLoop();
    virtual ~MessageLoop();

    bool PostTask(TaskBasePtr task);
    void PostDelayTask(TaskBasePtr t, uint32_t milliseconds);

    /* Task will run in target loop thread,
     * reply will run in Current() loop if it exist,
     * otherwise the reply task will run in target messageloop thread too*/
    bool PostTaskAndReply(StlClosure task, StlClosure reply);
    bool PostTaskAndReply(TaskBasePtr task, TaskBasePtr reply);
    bool PostTaskAndReply(TaskBasePtr task,
                          TaskBasePtr reply,
                          MessageLoop* reply_queue);

    void Start();
    bool IsInLoopThread() const;

    //t: millsecond for giveup cpu for waiting
    void WaitLoopEnd(int32_t t = 1);
    void SetLoopName(std::string name);
    const std::string& LoopName() const {return loop_name_;}

    void QuitLoop();
    EventPump* Pump() {return &event_pump_;}

    void BeforePumpRun() override;
    void AfterPumpRun() override;
  private:
    void ThreadMain();
    void SetThreadNativeName();
    class ReplyTaskHelper;

    void RunCommandTask(ScheduledTaskType t);

    // nested task: post another task in current loop
    // override from pump for nested task;
    void RunNestedTask() override;
    void RunTimerClosure(const TimerEventList&) override;

    void ScheduleFutureReply(std::shared_ptr<ReplyHolder>& reply);

    int Notify(int fd, const void* data, size_t count);
  private:

    std::atomic_int status_;
    std::atomic_int has_join_;
    std::atomic_flag running_;

    std::mutex start_stop_lock_;
    std::condition_variable cv_;

    std::string loop_name_;
    std::unique_ptr<std::thread> thread_ptr_;

    int task_fd_ = -1;
    RefFdEvent task_event_;
    std::atomic_flag notify_flag_;
    ConcurrentTaskQueue scheduled_tasks_;
    std::list<TaskBasePtr> in_loop_tasks_;

    int reply_event_fd_ = -1;
    RefFdEvent reply_event_;
    base::SpinLock future_reply_lock_;
    std::list<std::shared_ptr<ReplyHolder>> future_replys_;

    // pipe just use for loop control
    int wakeup_pipe_in_ = -1;
    int wakeup_pipe_out_ = -1;
    RefFdEvent wakeup_event_;

    EventPump event_pump_;
};

} //end namespace
#endif
