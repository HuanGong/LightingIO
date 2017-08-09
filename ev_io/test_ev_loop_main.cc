


#include "ev_loop.h"
#include "cstdlib"
#include "unistd.h"

#include <iostream>

#include "ev_task.h"


int main() {

  IO::EventLoop loop("task loop");
  std::cout << "is current thread:" << loop.IsCurrent() << std::endl;
  loop.Start();
  std::cout << "is current thread:" << loop.IsCurrent() << std::endl;
  loop.PostTask(IO::NewClosure([]() {
    std::cout << "run in task loop" << std::endl;
    std::cout << std::this_thread::get_id() << std::endl;
  }));
  std::cout << "loop end" << std::endl;
  return 0;
}
