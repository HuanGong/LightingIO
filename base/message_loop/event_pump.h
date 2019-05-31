#ifndef BASE_EVENT_PUMP_H_
#define BASE_EVENT_PUMP_H_

#include <vector>
#include <memory>
#include <map>
#include <inttypes.h>
#include <thread>

#include "fd_event.h"
#include "timeout_event.h"
#include <thirdparty/timeout/timeout.h>

namespace base {

class IOMux;

typedef struct timeouts TimeoutWheel;
typedef std::function<void()> QuitClosure;
typedef std::shared_ptr<FdEvent> RefFdEvent;
typedef std::vector<TimeoutEvent*> TimerEventList;

class PumpDelegate {
public:
  virtual void BeforePumpRun() {};
  virtual void AfterPumpRun() {};

  virtual void RunScheduledTask() {};
  virtual void RunTimerClosure(const TimerEventList&) {};
  virtual bool LoopImmediate() const {return false;};
};

/* pump fd event and timeout event and pass them to handler by delegate interface*/
class EventPump : public FdEvent::FdEventWatcher {
public:
  EventPump();
  EventPump(PumpDelegate* d);
  ~EventPump();

  void Run();
  void Quit();

  bool InstallFdEvent(FdEvent *fd_event);
  bool RemoveFdEvent(FdEvent* fd_event);

  // override from FdEventWatcher
  void OnEventChanged(FdEvent* fd_event) override;

  void AddTimeoutEvent(TimeoutEvent* timeout_ev);
  void RemoveTimeoutEvent(TimeoutEvent* timeout_ev);

  QuitClosure Quit_Clourse();
  bool IsInLoopThread() const;

  bool Running() { return running_; }
  void SetLoopThreadId(std::thread::id id) {tid_ = id;}
protected:
  inline FdEvent::FdEventWatcher* AsFdWatcher() {return this;}
  /* update the time wheel mononic time and get all expired
   * timeoutevent will be invoke and re shedule if was repeated*/
  void ProcessTimerEvent();

  /* return the possible minimal wait-time for next timer
   * return 0 when has event expired,
   * return default_timeout when no timer*/
  timeout_t NextTimerTimeout(timeout_t default_timeout);

  /* finalize TimeoutWheel and delete Life-Ownered timeoutEvent*/
  void InitializeTimeWheel();
  void FinalizeTimeWheel();

  /*calculate abs timeout time and add to timeout wheel*/
  void add_timer_internal(uint64_t now_us, TimeoutEvent* event);
private:
  PumpDelegate* delegate_;
  bool running_;

  std::unique_ptr<IOMux> multiplexer_;

#if 0
  // replace by new timeout_wheel for more effective perfomance [ O(1) ]
  //TimerTaskQueue timer_queue_;
#endif
  TimeoutWheel* timeout_wheel_ = NULL;

  std::thread::id tid_;
};

}
#endif
