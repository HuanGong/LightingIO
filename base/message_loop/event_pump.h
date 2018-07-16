#ifndef BASE_EVENT_PUMP_H_
#define BASE_EVENT_PUMP_H_

#include <vector>
#include <memory>
#include <map>
#include <inttypes.h>
#include <thread>

#include "timer_task_queue.h"
#include "fd_event.h"

namespace base {

class IoMultiplexer;

typedef std::function<void()> QuitClourse;
typedef std::shared_ptr<FdEvent> RefFdEvent;

class PumpDelegate {
public:
  virtual void RunNestedTask() {};
  virtual void BeforePumpRun() {};
  virtual void AfterPumpRun() {};
};

class EventPump : public FdEvent::FdEventWatcher {
public:
  EventPump();
  EventPump(PumpDelegate* d);
  ~EventPump();

  void Run();
  void Quit();

  bool InstallFdEvent(FdEvent *fd_event);
  bool RemoveFdEvent(FdEvent* fd_event);

  void UpdateFdEvent(FdEvent* fd_event) override;

  bool CancelTimer(uint32_t timer_id);
  int32_t ScheduleTimer(RefTimerEvent& timerevent);

  QuitClourse Quit_Clourse();
  bool IsInLoopThread() const;

  bool Running() { return running_; }
  void SetLoopThreadId(std::thread::id id) {tid_ = id;}
protected:
  int64_t HandleTimerTask();
  inline FdEvent::FdEventWatcher* AsFdEventDelegate() {return this;}

private:
  PumpDelegate* delegate_;
  bool running_;

  std::vector<FdEvent*> active_events_;
  std::unique_ptr<IoMultiplexer> multiplexer_;

  TimerTaskQueue timer_queue_;
  std::thread::id tid_;
};

}
#endif
