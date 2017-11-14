#ifndef BASE_EVENT_PUMP_H_
#define BASE_EVENT_PUMP_H_

#include <vector>
#include <memory>
#include <map>
#include <inttypes.h>

namespace base {

class FdEvent;
class IoMultiplexer;

typedef std::shared_ptr<FdEvent> FdEventPtr;
typedef std::function<void()> QuitClourse;

class EventPump {
public:
  EventPump();
  ~EventPump();

  void Run();
  void Quit();

  void InstallFdEvent(FdEvent *fd_event);

  bool Running() { return running_; }

  QuitClourse Quit_Clourse();
protected:
  //void ScheduleWork();
  void ScheduleTimerTask();
private:
  //std::map<int64_t, > timer_events;
  bool running_;
  int32_t prefect_timeout_;

  std::vector<FdEvent*> active_events_;
  std::unique_ptr<IoMultiplexer> multiplexer_;
};

}
#endif
