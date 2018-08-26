#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <iostream>
#include <sys/epoll.h>
#include <atomic>
#include <memory>
#include "../fd_event.h"
#include "../event_pump.h"
#include "../timer_event.h"
#include "../message_loop.h"
#include "../linux_signal.h"
#include "base/coroutine/coroutine.h"
#include "base/coroutine/coroutine_runner.h"



bool MessageLoopTest();
bool FdEventTest();
bool TimerEventTest();
bool CoroutineTaskTest() {
  base::MessageLoop loop;

  uint64_t coro1_id = 0;
  std::weak_ptr<base::Coroutine> weak1;
  uint64_t coro2_id = 0;
  std::weak_ptr<base::Coroutine> weak2;

  loop.PostCoroTask([](){
    LOG(INFO) << "Task Run At Coroutine, task 1";
  });
  loop.PostCoroTask([&](){
    LOG(INFO) << "Task Run At Coroutine, task 2 start";
    base::RefCoroutine coro = base::CoroRunner::CurrentCoro();
    weak1 = coro;
    coro1_id = coro->Identify();
    base::CoroRunner::YieldCurrent();
    LOG(INFO) << "Task Run At Coroutine, task 2 resume";
  });
  loop.PostCoroTask([](){
    LOG(INFO) << "Task Run At Coroutine, task 3";
  });
  loop.PostCoroTask([&](){
    LOG(INFO) << "Task Run At Coroutine, task 4";
    base::RefCoroutine coro = base::CoroRunner::CurrentCoro();
    weak2 = coro;
    coro2_id = coro->Identify();
    base::CoroRunner::YieldCurrent();
    LOG(INFO) << "Task Run At Coroutine, task 4 resume";
  });

  loop.Start();
  loop.PostTask(base::NewClosure([](){
    LOG(INFO) << "Normal Task Run, task";
  }));

  loop.PostDelayTask(base::NewClosure([&](){
    LOG(INFO) << "resume coroutine 2 start";
    base::RefCoroutine coro = weak1.lock();
    coro->Resume(coro1_id);
    LOG(INFO) << "resume coroutine 2 finished";

    LOG(INFO) << "resume coroutine 4 start";
    base::RefCoroutine coro2 = weak2.lock();
    coro->Resume(coro2_id);
    LOG(INFO) << "resume coroutine 4 finished";
  }), 1000);

  loop.PostDelayTask(base::NewClosure([&](){
    LOG(INFO) << "Quit Loop";
    loop.QuitLoop();
  }), 10*1000);

  loop.WaitLoopEnd();
  return  true;
}

int main(int argc, char** argv) {
  //google::InitGoogleLogging(argv[0]);  // 初始化 glog
  //google::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags

  //FdEventTest();
  //TimerEventTest();
  //static void signal(int sig, const std::function<void()>& handler);


  //MessageLoopTest();
  CoroutineTaskTest();
  return 0;
}

bool MessageLoopTest() {
  base::MessageLoop loop;
  loop.SetLoopName("main_loop");
  loop.Start();

  base::MessageLoop reply_loop;
  reply_loop.SetLoopName("reply_loop");
  reply_loop.Start();
  {

    loop.PostTaskAndReply(base::NewClosure([&](){
        LOG(INFO) << "PostTaskAndReply task run";
      }), base::NewClosure([&]() {
        LOG(INFO) << "PostTaskAndReply reply run";
        reply_loop.Pump()->Quit();
        loop.PostTask(base::NewClosure([&]() {
          loop.Pump()->Quit();
        }));
      }), &reply_loop);
  }

  reply_loop.WaitLoopEnd();
  loop.WaitLoopEnd();
  return true;
}

bool FdEventTest() {
  base::EventPump loop;

  std::atomic<int> flag;
  flag.store(false);

  int fd[2];
  if (0 != pipe2(fd, O_CLOEXEC | O_NONBLOCK)) {
    std::cout << "Create O_NONBLOCK pipe Failed" << std::endl;
    return false;
  }
  int read_fd = fd[0];
  int write_fd = fd[1];
  std::cout << "read fd" << read_fd << " write fd:" << write_fd << std::endl;

  base::FdEvent fd_ev(read_fd, EPOLLIN | EPOLLRDHUP | EPOLLHUP);
  fd_ev.SetReadCallback([&]() {
    int value = 0;
    int count = read(fd_ev.fd(), (char*)&value, sizeof(int));
    std::cout << "Get " << count << " bytes data, Value:" << value << std::endl;

    fd_ev.DisableReading();
    //loop.RemoveFdEvent(&fd_ev);
    //loop.Quit();
  });

  fd_ev.SetCloseCallback([&]() {
    LOG(INFO) << "fd was be closed, you should do your close action here, reconnection or clean up";
    //case 1; clean up,
    loop.RemoveFdEvent(&fd_ev);
    //case 2; reconnection
  });

  std::thread thread([&](){
    int times = 10;
    sleep(1);
    int count = write(write_fd, &times, sizeof(times));
    std::cout << "write " << count << " bytes data to fd" << std::endl;

    sleep(2);
    count = write(write_fd, &times, sizeof(times));
    std::cout << "write " << count << " bytes data to fd" << std::endl;

  });

  loop.InstallFdEvent(&fd_ev);
  loop.Run();

  if (thread.joinable()) {
    thread.join();
  }
  close(write_fd);
  //close(read_fd); owner by FdEvent
  return 0;
}

bool TimerEventTest() {
  {
    base::TimerTaskQueue q;
    q.TestingPriorityQueue();
  }

  base::EventPump loop;
  int repeat_count = 0;

  {
    base::RefTimerEvent timer = std::make_shared<base::TimerEvent>(5000);
    uint32_t timer_id = loop.ScheduleTimer(timer);
    timer->SetTimerTask(base::NewClosure([&]() {
      LOG(INFO) << "timer1(once) Invoked";
    }));
    timer_id = timer_id; //jsut make gcc happy
  }

  {
    base::RefTimerEvent timer2 = std::make_shared<base::TimerEvent>(2000, false);
    uint32_t timer_id2 = loop.ScheduleTimer(timer2);
    timer2->SetTimerTask(base::NewClosure([&]() {
      LOG(INFO) << "timer2(repeated) Invoked";
      if (3 == (++repeat_count)) {
        loop.CancelTimer(timer_id2);
      }
    }));
  }

  {
    base::RefTimerEvent timer3 = std::make_shared<base::TimerEvent>(10000);
    uint32_t timer_id3 = loop.ScheduleTimer(timer3);
    timer3->SetTimerTask(base::NewClosure([&]() {
      LOG(INFO) << "timer3( for quit ) Invoked";
      loop.Quit();
    }));
    timer_id3 = timer_id3; //make gcc happy
  }

  loop.Run();
  return true;
}
