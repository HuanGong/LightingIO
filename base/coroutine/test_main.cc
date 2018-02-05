

#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include <event_loop/msg_event_loop.h>
#include <unistd.h>


void coro_fun() {
  //LOG(INFO) << "coro_fun enter,";
  usleep(1000);
  //LOG(INFO) << "coro_fun leave,";
}

int main() {

  std::vector<base::MessageLoop2*> loops;
  for (int i = 0; i < 4; i++) {
    base::MessageLoop2* loop = new base::MessageLoop2();
    loop->SetLoopName(std::to_string(i));
    loop->Start();
    loops.push_back(loop);
  }


  int i = 0;
  do {
    base::MessageLoop2* loop = loops[i % loops.size()];

    loop->PostTask(base::NewClosure([]() {
      auto functor = std::bind(coro_fun);


      base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(functor));

      LOG(INFO) << "coro create and excuted success";
    }));
  } while(i++ < 100000);

  loops[0]->WaitLoopEnd();
  return 0;
}
