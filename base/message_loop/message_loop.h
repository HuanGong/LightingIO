#ifndef MESSAGE_LOOP_H_H
#define MESSAGE_LOOP_H_H

#include <atomic>
#include <thread>
#include <queue>
#include <mutex>

#include "event.h"
#include "closure/closure_task.h"

#include "memory/scoped_ref_ptr.h"
#include "memory/refcountedobject.h"

#include "glog/logging.h"

#include <iostream>

namespace base {

enum LoopState {
  ST_STOP = 0,
  ST_INITING = 1,
  ST_INITTED = 2,
  ST_STARTED = 3
};

class MessageLoop {
  public:
    MessageLoop(std::string name);
    virtual ~MessageLoop();

    void PostTask(std::unique_ptr<QueuedTask> task);
    void PostDelayTask(std::unique_ptr<QueuedTask> t, uint32_t milliseconds);

    void PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                          std::unique_ptr<QueuedTask> reply,
                          MessageLoop* reply_queue);
    bool PostTaskAndReply(std::unique_ptr<QueuedTask> task,
                          std::unique_ptr<QueuedTask> reply);

    void Start();
    bool IsInLoopThread() const;
    static MessageLoop* Current();

    event_base* EventBase() {return event_base_;}
  private:
    void ThreadMain();
    static void OnWakeup(int socket, short flags, void* context);  // NOLINT
    static void RunTask(int fd, short flags, void* context);       // NOLINT
    static void RunTimer(int fd, short flags, void* context);      // NOLINT

    //struct LoopContext;
    class ReplyTaskOwner;
    class PostAndReplyTask;
    class SetTimerTask;

    typedef RefCountedObject<ReplyTaskOwner> ReplyTaskOwnerRef;
    void PrepareReplyTask(scoped_refptr<ReplyTaskOwnerRef> reply_task);

    event_base* event_base_;

    int wakeup_pipe_in_ = -1;
    int wakeup_pipe_out_ = -1;

    std::mutex pending_lock_;

    std::unique_ptr<event> wakeup_event_;
    std::list<std::unique_ptr<QueuedTask>> pending_;
    std::list<scoped_refptr<ReplyTaskOwnerRef>> pending_replies_;

    std::atomic_int status_;

    std::thread::id tid_;
    std::string loop_name_;
    std::unique_ptr<std::thread> thread_ptr_;
};

}//endnamespace

#endif
