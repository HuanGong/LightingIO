#include <catch/catch.hpp>

#include <iostream>
#include "message_loop/event_pump.h"
#include "message_loop/message_loop.h"

void FailureDump(const char* s, int sz) {
  std::string failure_info(s, sz);
  LOG(INFO) << " ERROR: " << failure_info;
}

TEST_CASE("event_pump.timer", "[test event pump timer]") {

  base::EventPump pump;
  pump.SetLoopThreadId(std::this_thread::get_id());

  uint64_t repeated_timer_checker = 0;
  base::TimeoutEvent* repeated_toe = new base::TimeoutEvent(5000, true);
  repeated_toe->InstallTimerHandler(NewClosure([&]() {
    uint64_t t = base::time_ms();
    uint64_t diff = t - repeated_timer_checker;
    REQUIRE(((diff >= 5000) && (diff <= 5001)));
    repeated_timer_checker = t;
  }));

  uint64_t timer_ms_checker = 0;
  base::TimeoutEvent* oneshot_toe = new base::TimeoutEvent(5, false);
  oneshot_toe->InstallTimerHandler(NewClosure([&]() {
    uint64_t t = base::time_ms();
    uint64_t diff = t - timer_ms_checker;
    REQUIRE((diff >= 5 && diff <= 6));
  }));

  uint64_t timer_zero_checker = 0;
  base::TimeoutEvent* zerooneshot_toe = new base::TimeoutEvent(0, false);
  zerooneshot_toe->InstallTimerHandler(NewClosure([&]() {
    uint64_t t = base::time_ms();
    uint64_t diff = t - timer_zero_checker;
    REQUIRE((diff >= 0 && diff <= 1));
  }));

  uint64_t start, end;
  base::TimeoutEvent* quit_toe = base::TimeoutEvent::CreateSelfDeleteTimeoutEvent(30000);
  quit_toe->InstallTimerHandler(NewClosure([&]() {
    end = base::time_ms();
    std::cout << "end at ms:" << end << std::endl;
    REQUIRE(end - start >= 30000);
    pump.RemoveTimeoutEvent(repeated_toe);
    pump.RemoveTimeoutEvent(oneshot_toe);
    pump.RemoveTimeoutEvent(zerooneshot_toe);
    pump.RemoveTimeoutEvent(quit_toe);
    pump.Quit();
  }));

  repeated_timer_checker = timer_ms_checker = timer_zero_checker = start = base::time_ms();
  std::cout << "start at ms:" <<  timer_ms_checker << std::endl;
  pump.AddTimeoutEvent(oneshot_toe);
  pump.AddTimeoutEvent(repeated_toe);
  pump.AddTimeoutEvent(zerooneshot_toe);
  pump.AddTimeoutEvent(quit_toe);


  pump.Run();

  delete repeated_toe;
  delete oneshot_toe;
  delete zerooneshot_toe;
}

TEST_CASE("messageloop.delaytask", "[run delay task]") {
  base::MessageLoop loop;
  loop.SetLoopName("DelayTaskTestLoop");
  loop.Start();

  uint64_t start = base::time_ms();
  loop.PostDelayTask(NewClosure([&]() {
    uint64_t end = base::time_ms();
    REQUIRE(((end - start >= 500) && (end - start <= 501)));
  }), 500);

  loop.PostDelayTask(NewClosure([&]() {
    loop.QuitLoop();
  }), 1000);

  loop.WaitLoopEnd();
}

TEST_CASE("messageloop.replytask", "[task with reply function]") {

  base::MessageLoop loop;
  loop.Start();

  base::MessageLoop replyloop;
  replyloop.Start();

  bool inloop_reply_run = false;
  bool outloop_reply_run = false;

  loop.PostTaskAndReply(NewClosure([&]() {
    LOG(INFO) << " task bind reply in loop run";
  }), NewClosure([&]() {
    inloop_reply_run = true;
    REQUIRE(base::MessageLoop::Current() == &loop);
  }));

  loop.PostTaskAndReply(NewClosure([&]() {
    LOG(INFO) << " task bind reply use another loop run";
  }), NewClosure([&]() {
    outloop_reply_run = true;
    REQUIRE(base::MessageLoop::Current() == &replyloop);
  }), &replyloop);


  loop.PostDelayTask(NewClosure([&](){
    loop.QuitLoop();
  }), 2000);
  loop.WaitLoopEnd();
  REQUIRE(inloop_reply_run);
  REQUIRE(outloop_reply_run);
}


TEST_CASE("messageloop.tasklocation", "[new task tracking location ]") {
  google::InstallFailureSignalHandler();
  google::InstallFailureWriter(FailureDump);

  base::MessageLoop loop;
  loop.Start();

  loop.PostTask(NewClosure([&]() {
    LOG(INFO) << " task_location exception by throw";
    //throw "task throw";
  }));

  loop.PostDelayTask(NewClosure([&](){
    loop.QuitLoop();
  }), 2000);
  loop.WaitLoopEnd();
}
