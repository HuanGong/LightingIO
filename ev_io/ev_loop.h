#ifndef IO_EV_LOOP_H_H
#define IO_EV_LOOP_H_H

#include "event.h"
#include <atomic>
#include <thread>

#include <iostream>
#include <queue>

#include <mutex>

#include "ev_task.h"

namespace IO {

enum LoopState {
  ST_STOP = 0,
  ST_INITING = 1,
  ST_INITTED = 2,
  ST_STARTED = 3
};

class EventLoop {
  public:
    EventLoop(std::string name);
    ~EventLoop();

    void Start();
    bool IsCurrent();
    EventLoop& Current();
  private:
    void LoopMain();
    static void OnWakeup(int socket, short flags, void* context);  // NOLINT
    static void RunTask(int fd, short flags, void* context);       // NOLINT
    static void RunTimer(int fd, short flags, void* context);      // NOLINT

    //class ReplyTaskOwner;
    class PostAndReplyTask;
    class SetTimerTask;

    //typedef RefCountedObject<ReplyTaskOwner> ReplyTaskOwnerRef;

    //void PrepareReplyTask(scoped_refptr<ReplyTaskOwnerRef> reply_task);

    struct QueueContext;

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

} //endnamespace
#endif
