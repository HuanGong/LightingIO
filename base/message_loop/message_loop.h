#ifndef MESSAGE_LOOP_H_H
#define MESSAGE_LOOP_H_H

#include <atomic>
#include <thread>
#include <queue>
#include <mutex>

#include "event.h"
#include "closure_task.h"

#include <iostream>

namespace base {

enum LoopState {
  ST_STOP = 0,
  ST_INITING = 1,
  ST_INITTED = 2,
  ST_STARTED = 3
};

#ifdef XXXX_ENABLE_SET_TIMMER
class SetTimerTask : public QueuedTask {
 public:
  SetTimerTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds)
      : task_(std::move(task)),
        milliseconds_(milliseconds),
        posted_(time_ms()) {}

 private:
  bool Run() override {
    uint32_t post_time = time_ms() - posted_;
    //TaskQueue::Current()->PostDelayedTask(std::move(task_),
    //                                      post_time > milliseconds_ ? 0 : milliseconds_ - post_time);
    return true;
  }

  std::unique_ptr<QueuedTask> task_;
  const uint32_t milliseconds_;
  const uint32_t posted_;
};
#endif

class MessageLoop {
  public:
    MessageLoop(std::string name);
    virtual ~MessageLoop();

    void PostTask(std::unique_ptr<QueuedTask> task);
    void PostDelayTask(std::unique_ptr<QueuedTask> t, uint32_t milliseconds);

    void Start();
    bool IsInLoopThread() const;
    static MessageLoop* Current();

    event_base* EventBase() {return event_base_;}
  private:
    void ThreadMain();
    static void OnWakeup(int socket, short flags, void* context);  // NOLINT
    static void RunTask(int fd, short flags, void* context);       // NOLINT
    static void RunTimer(int fd, short flags, void* context);      // NOLINT

    //class ReplyTaskOwner;
    //class PostAndReplyTask;
    //class SetTimerTask;

    //typedef RefCountedObject<ReplyTaskOwner> ReplyTaskOwnerRef;
    //void PrepareReplyTask(scoped_refptr<ReplyTaskOwnerRef> reply_task);
    //struct LoopContext;

    event_base* event_base_;

    int wakeup_pipe_in_ = -1;
    int wakeup_pipe_out_ = -1;

    std::mutex pending_lock_;

    std::unique_ptr<event> wakeup_event_;
    std::list<std::unique_ptr<QueuedTask>> pending_;
    //std::list<scoped_refptr<ReplyTaskOwnerRef>> pending_replies_

    std::atomic_int status_;

    std::thread::id tid_;
    std::string loop_name_;
    std::unique_ptr<std::thread> thread_ptr_;
};

}//endnamespace

#endif
