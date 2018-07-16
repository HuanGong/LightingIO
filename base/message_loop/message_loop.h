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

#include "spin_lock.h"
#include "closure/closure_task.h"
#include "memory/scoped_ref_ptr.h"
#include "memory/refcountedobject.h"

namespace base {

enum LoopState {
  ST_INITING = 1,
  ST_STARTED = 2,
  ST_STOPED  = 3
};

class ReplyHolder {
public:
  static std::shared_ptr<ReplyHolder> Create(std::unique_ptr<QueuedTask> task) {
    return std::make_shared<ReplyHolder>(std::move(task));
  }
  bool InvokeReply() {
    if (!reply_ || !commited_) {
      return false;
    }
    return reply_->Run();
  }
  inline void CommitReply() {commited_ = true;}
  inline bool IsCommited() {return commited_;};
  ReplyHolder(std::unique_ptr<QueuedTask> task) :
    commited_(false),
    reply_(std::move(task)) {
  }
private:
  bool commited_;
  std::unique_ptr<QueuedTask> reply_;
};

class MessageLoop : public PumpDelegate {
  public:
    static MessageLoop* Current();

    typedef enum {
      TaskTypeDefault = 0,
      TaskTypeReply   = 1
    } ScheduledTaskType;


    MessageLoop();
    virtual ~MessageLoop();

    bool PostTask(std::unique_ptr<QueuedTask> task);

    void PostDelayTask(std::unique_ptr<QueuedTask> t, uint32_t milliseconds);

    /* Task will run in target loop thread,
     * reply will run in Current() loop if it exist,
     * otherwise the reply task will run in target messageloop thread too*/
    bool PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                          std::unique_ptr<QueuedTask> reply);

    void PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                          std::unique_ptr<QueuedTask> reply,
                          MessageLoop* reply_queue);

    bool InstallSigHandler(int, const SigHandler);

    void Start();
    void WaitLoopEnd();
    bool IsInLoopThread() const;
    void SetLoopName(std::string name);
    std::string LoopName() {return loop_name_;}

    void QuitLoop();
    EventPump* Pump() { return event_pump_.get(); }

    void BeforePumpRun() override;
    void AfterPumpRun() override;
  private:
    class SetTimerTask;
    class BindReplyTask;

    void ThreadMain();

    void OnWakeup();  // NOLINT
    void RunScheduledTask(ScheduledTaskType t);

    // nested task: in a inloop task , post another task in current loop
    // override from pump for nested task;
    void RunNestedTask() override;
    void ScheduleFutureReply(std::shared_ptr<ReplyHolder>& reply);

  private:
    std::atomic_int status_;
    std::atomic_int has_join_;
    std::atomic_flag running_;

    std::mutex pending_lock_;

    std::string loop_name_;
    std::unique_ptr<std::thread> thread_ptr_;

    //new version >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    int task_event_fd_ = -1;
    RefFdEvent task_event_;
    base::SpinLock task_lock_;
    std::list<std::unique_ptr<QueuedTask>> scheduled_task_;

    int reply_event_fd_ = -1;
    RefFdEvent reply_event_;
    base::SpinLock future_reply_lock_;
    std::list<std::shared_ptr<ReplyHolder>> future_replys_;

    // those task was post inloop thread, no lock needed
    std::list<std::unique_ptr<QueuedTask>> inloop_scheduled_task_;

    //new version <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    int wakeup_pipe_in_ = -1;
    int wakeup_pipe_out_ = -1;
    RefFdEvent wakeup_event_;
    std::unique_ptr<EventPump> event_pump_;

    std::map<int, SigHandler> sig_handlers_;
};

} //end namespace
#endif
