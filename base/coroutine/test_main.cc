

#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include <event_loop/msg_event_loop.h>
#include <unistd.h>

intptr_t id = 0;

void coro_fun() {
  LOG(INFO) << "coro_fun enter, going to Yield";
  id = base::CoroScheduler::TlsCurrent()->CurrentCoroId();
  base::CoroScheduler::TlsCurrent()->YieldCurrent();
  LOG(INFO) << "coro_fun end, after be resume";
}

void resume_coro(intptr_t id) {
  base::CoroScheduler::TlsCurrent()->ResumeCoroutine(id);
  LOG(INFO) << "back to main coroutine after resume 'kPaused' coro_fun";
}

int main() {

  base::MessageLoop2 loop;
  loop.Start();

  loop.PostTask(base::NewClosure(
      std::bind([&]() {

        auto functor = std::bind(coro_fun);
        base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(functor));

        LOG(INFO) << "coro_fun seems Yield, schedule a function to resume it";
        auto re = std::bind(resume_coro, id);
        loop.PostTask(base::NewClosure(re));

      })));

  //loop.WaitLoopEnd();
  sleep(10);
  return 0;
}
