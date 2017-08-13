#include <iostream>
#include "message_loop.h"

#include "time.h"
#include "glog/logging.h"


int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  LOG(ERROR) << "main messageloop test run";

  base::MessageLoop io("ioloop");
  base::MessageLoop worker("workerloop");
  io.Start();
  worker.Start();

  sleep(1);
  worker.PostTask(base::NewClosure([]() {
    LOG(ERROR) << "this should run in worker thread";
  }));

  sleep(1);
  io.PostTask(base::NewClosure([]() {
    LOG(ERROR) << "this should run in IO thread";
  }));

  LOG(ERROR) << "post a timeout task 1000ms";
  io.PostDelayTask(base::NewClosure([]() {
    DLOG(ERROR) << "this is a timer thread";
  }), 1000);

  sleep(5);
  LOG(ERROR) << "post a task with reply";
  io.PostTaskAndReply(base::NewClosure([]() {
    LOG(ERROR) << "this run on io thread";
  }),
  base::NewClosure([]() {
    LOG(ERROR) << "this replay run on worker thread";
  }),
  &worker);

  sleep(5);
  LOG(ERROR) << "post a timeout task 1000ms";
  io.PostTaskAndReply(base::NewClosure([]() {
    LOG(ERROR) << "this run on io thread";
  }),
                      base::NewClosure([]() {
                        LOG(ERROR) << "this replay run on worker thread";
                      }));
  bool start = false, running = true;
  int posts[20] = {0};
  int excuted = 0;
  std::vector<std::thread*> ths;

  auto f = [&](int i) {
    while(running) {
      if (!start) {
        sleep(0);
        continue;
      }
      posts[i]++;
      io.PostTask(base::NewClosure([&]() {
        if (start)
          excuted++;
      }));
    }
  };

  for (int i = 0; i < 20; i++) {
    ths.push_back(new std::thread(std::bind(f, i)));
  }

  LOG(ERROR) << "\n      start >>>>";
  start = true;
  sleep(2);
  start = false;
  LOG(ERROR) << "\n   end test <<<<<";
  running = false;
  for (auto p : ths)
    p->join();

  long count = 0;
  for (int i = 0; i < 20; i++) {
    count += posts[i];
  }
  LOG(ERROR) << " total post :" << count << " task, excuted:" << excuted;

  while(1) {
    sleep(2);
  }
  return 0;
}
