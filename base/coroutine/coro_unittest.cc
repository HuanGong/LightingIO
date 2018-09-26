
#include "coroutine.h"
#include "glog/logging.h"
#include "coroutine_runner.h"
#include <message_loop/message_loop.h>
#include <unistd.h>
#include <time/time_utils.h>

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>

#define CATCH_CONFIG_MAIN //only once
#include <catch/catch.hpp>

base::MessageLoop mainloop;
base::MessageLoop taskloop;

void coro_f() {
  LOG(INFO) << " coro_f enter";
  usleep(1000);
  LOG(INFO) << " coro_f leave";
}

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
}

TEST_CASE("coro_base", "[test coroutine base with message loop]") {
  mainloop.Start();
  taskloop.Start();
  TestCoroutineWithLoop();
}

TEST_CASE("go_coro", "[go flag call coroutine]") {
  base::MessageLoop loop;
  base::MessageLoop loop2;
  loop.Start();
  loop2.Start();

  loop.PostTask(base::NewClosure([&]() {
    LOG(INFO) << " start test go flag enter";

    go coro_f;

    go std::bind(&coro_fun, "tag_from_go_synatax");

    go []() {
      LOG(INFO) << " run lambda in coroutine";
    };
    LOG(INFO) << " start test go flag leave";
  }));
  loop.WaitLoopEnd();
  loop2.WaitLoopEnd();
}
