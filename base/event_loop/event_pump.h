#ifndef BASE_EVENT_PUMP_H_
#define BASE_EVENT_PUMP_H_

#include <vector>
#include <memory>
#include <map>
#include <inttypes.h>

#include "timer_task_queue.h"
#include "fd_event.h"

namespace base {

class IoMultiplexer;

typedef std::function<void()> QuitClourse;
typedef std::shared_ptr<FdEvent> RefFdEvent;

class EventPump : public FdEvent::Delegate {
public:
  EventPump();
  ~EventPump();

  void Run();
  void Quit();

  void InstallFdEvent(FdEvent *fd_event);
  void RemoveFdEvent(FdEvent* fd_event);

  void UpdateFdEvent(FdEvent* fd_event) override;

  int32_t ScheduleTimer(RefTimerEvent& timerevent);
  bool CancelTimer(uint32_t timer_id);

  bool Running() { return running_; }

  QuitClourse Quit_Clourse();

  inline FdEvent::Delegate* AsFdEventDelegate() {
    return this;
  }
protected:
  int64_t HandleTimerTask();
private:
  bool running_;
  int32_t prefect_timeout_;

  std::vector<FdEvent*> active_events_;
  std::unique_ptr<IoMultiplexer> multiplexer_;

  TimerTaskQueue timer_queue_;
};

}
#endif
