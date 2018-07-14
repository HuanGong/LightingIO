#ifndef _BASE_MSG_EVENT_LOOP_H_H
#define _BASE_MSG_EVENT_LOOP_H_H

#include <atomic>
#include <thread>
#include <queue>
#include <mutex>

#include "fd_event.h"
#include "event_pump.h"
#include "glog/logging.h"

#include "closure/closure_task.h"
#include "memory/scoped_ref_ptr.h"
#include "memory/refcountedobject.h"

namespace base {

enum LoopState {
  ST_INITING = 1,
  ST_STARTED = 2,
  ST_STOPED  = 3
};

class MessageLoop2 : public PumpDelegate {
  public:
    static MessageLoop2* Current();

    MessageLoop2();
    virtual ~MessageLoop2();

    void PostTask(std::unique_ptr<QueuedTask> task);

    void PostDelayTask(std::unique_ptr<QueuedTask> t,
                       uint32_t milliseconds);

    void PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                          std::unique_ptr<QueuedTask> reply,
                          MessageLoop2* reply_queue);
    bool PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                          std::unique_ptr<QueuedTask> reply);

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
    void ThreadMain();

    void OnWakeup();  // NOLINT

    //struct LoopContext;
    class ReplyTaskOwner;
    class PostAndReplyTask;
    class SetTimerTask;

    typedef RefCountedObject<ReplyTaskOwner> ReplyTaskOwnerRef;
    void PrepareReplyTask(scoped_refptr<ReplyTaskOwnerRef> reply_task);

  private:
    std::atomic_int status_;
    std::atomic_int has_join_;
    std::atomic_flag running_;

    std::mutex pending_lock_;

    std::list<std::unique_ptr<QueuedTask>> pending_;
    std::list<scoped_refptr<ReplyTaskOwnerRef>> pending_replies_;

    std::string loop_name_;
    std::unique_ptr<std::thread> thread_ptr_;

    int wakeup_pipe_in_ = -1;
    int wakeup_pipe_out_ = -1;
    RefFdEvent wakeup_event_;
    std::unique_ptr<EventPump> event_pump_;

    std::map<int, SigHandler> sig_handlers_;
};

} //end namespace
#endif
