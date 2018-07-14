

#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include <message_loop/message_loop.h>
#include <unistd.h>


void coro_fun() {
  //LOG(INFO) << "coro_fun enter,";
  usleep(1000);
  //LOG(INFO) << "coro_fun leave,";
}

int main() {

  std::vector<base::MessageLoop*> loops;
  for (int i = 0; i < 4; i++) {
    base::MessageLoop* loop = new base::MessageLoop();
    loop->SetLoopName(std::to_string(i));
    loop->Start();
    loops.push_back(loop);
  }


  int i = 0;
  do {
    base::MessageLoop* loop = loops[i % loops.size()];

    loop->PostTask(base::NewClosure([]() {

      base::CoroClosure functor = std::bind(coro_fun);

      base::CoroScheduler::CreateAndSchedule(functor);

      LOG(INFO) << "coro create and excuted success";
    }));
  } while(i++ < 100000);

  loops[0]->WaitLoopEnd();
  return 0;
}
