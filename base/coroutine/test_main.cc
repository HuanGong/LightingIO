
#include "coroutine.h"
#include "glog/logging.h"
#include "coroutine_runner.h"
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

void TestCoroutineWithLoop() {
  int i = 0;
  std::string tag("WithLoop");

  //auto start = base::time_us();
  do {
      base::CoroClosure functor = std::bind(coro_fun, tag);
      mainloop.PostCoroTask(functor);
      taskloop.PostCoroTask(functor);
  } while(i++ < 10);

  mainloop.PostDelayTask(base::NewClosure([]() {
    mainloop.QuitLoop();
  }), 1000);

  taskloop.PostDelayTask(base::NewClosure([]() {
    mainloop.QuitLoop();
  }), 1000);

  mainloop.WaitLoopEnd();
  taskloop.WaitLoopEnd();
  //auto end = base::time_us();
}


int main() {
  mainloop.Start();
  taskloop.Start();

  TestCoroutineWithLoop();
  return 0;
}
