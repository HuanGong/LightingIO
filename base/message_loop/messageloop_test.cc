#include <iostream>
#include "message_loop.h"


int main() {

  base::MessageLoop io("ioloop");
  io.Start();

  base::MessageLoop worker("workerloop");
  worker.Start();

  worker.PostTask(base::NewClosure([]() {
    std::cout << "worker hello world message on thread:" << std::this_thread::get_id() << std::endl;
  }));

  io.PostTask(base::NewClosure([]() {
    std::cout << "io NORMAL task run on thread:" << std::this_thread::get_id() << std::endl;
  }));
  
  io.PostTaskAndReply(base::NewClosure([]() {
  }), 
  base::NewClosure([]() {
    std::cout << "this is reply task run on thread worker:" << std::endl;
  }),
  &worker); 
  
  while(1) {
    io.PostDelayTask(base::NewClosure([]() {
      std::cout << "io timeout task run on thread:" << std::this_thread::get_id() << std::endl;
    }), 1000);
    sleep(2);
  }
  return 0;
}
