#include <unistd.h>
#include "coroutine.h"
#include "glog/logging.h"
#include "coroutine_runner.h"
#include <time/time_utils.h>
#include <message_loop/message_loop.h>

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>

#define CATCH_CONFIG_MAIN //only once
#include <catch/catch.hpp>

void coro_c_function() {
  LOG(INFO) << __FUNCTION__ << " enter";
  LOG(INFO) << __FUNCTION__ << " leave";
}

void coro_fun(std::string tag) {
  LOG(INFO) << tag << " coro_fun enter";
  LOG(INFO) << tag << " coro_fun leave";
}

TEST_CASE("go_coro", "[go flag call coroutine]") {
  base::MessageLoop loop;
  loop.Start();

  loop.PostTask(NewClosure([&]() {
    LOG(INFO) << " start test go flag enter";

    go coro_c_function;

    go std::bind(&coro_fun, "tag_from_go_synatax");

    go []() {
      LOG(INFO) << " run lambda in coroutine" ;
    };
    LOG(INFO) << " start test go flag leave";
  }));

  go &loop << []() {
    LOG(INFO) << "go coroutine in loop ok!!!";
  };

  loop.PostDelayTask(NewClosure([&](){
    loop.QuitLoop();
  }), 2000);
  loop.WaitLoopEnd();
}

void FailureDump(const char* s, int sz) {
  std::string failure_info(s, sz);
  LOG(INFO) << " ERROR: " << failure_info;
}

TEST_CASE("task_location", "[new task tracking location ]") {
  google::InstallFailureSignalHandler();
  google::InstallFailureWriter(FailureDump);

  base::MessageLoop loop;
  loop.Start();

  loop.PostTask(NewClosure([&]() {
    LOG(INFO) << " task_location exception by throw";
    throw "task throw";
  }));

  loop.PostDelayTask(NewClosure([&](){
    loop.QuitLoop();
  }), 2000);
  loop.WaitLoopEnd();
}
