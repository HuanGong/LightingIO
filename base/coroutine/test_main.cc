
#include "coroutine.h"
#include "glog/logging.h"
#include "coroutine_scheduler.h"
#include <message_loop/message_loop.h>
#include <unistd.h>
#include <time/time_utils.h>

base::MessageLoop mainloop;
base::MessageLoop taskloop;

void coro_fun(std::string tag) {
  LOG(INFO) << tag << " coro_fun enter";
  usleep(1000);
  LOG(INFO) << tag << " coro_fun leave";
}

void TestCoroutine() {
  auto main = base::Coroutine::Create(true);
  auto sub = base::Coroutine::Create();
  std::string tag("TestTransfer");
  sub->SetCoroTask(std::bind(coro_fun, tag));

  coro_transfer(main.get(), sub.get());
}

void TestCoroutineWithLoop() {
  int i = 0;
  std::string tag("WithLoop");

  //auto start = base::time_us();
  do {
    mainloop.PostTask(base::NewClosure([&]() {
      base::CoroClosure functor = std::bind(coro_fun, tag);
      base::CoroScheduler::CreateAndTransfer(functor);
      LOG(INFO) << "coro create and excuted in main loop success";
    }));

    taskloop.PostTask(base::NewClosure([&]() {
      base::CoroClosure functor = std::bind(coro_fun, tag);
      base::CoroScheduler::CreateAndTransfer(functor);
      LOG(INFO) << "coro create and excuted in task loop success";
    }));
  } while(i++ < 10);

  mainloop.PostTask(base::NewClosure([]() {
    mainloop.QuitLoop();
  }));

  taskloop.PostTask(base::NewClosure([]() {
    mainloop.QuitLoop();
  }));

  mainloop.WaitLoopEnd();
  mainloop.WaitLoopEnd();

  //auto end = base::time_us();
}


int main() {
  mainloop.Start();
  taskloop.Start();

  TestCoroutine();
  return 0;
}
