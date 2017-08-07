


#include "ev_loop.h"
#include "cstdlib"
#include "unistd.h"

#include <iostream>
//#include <thread>


int main() {
  IO::EvIOLoop ioloop;
/*
  std::thread tr([&]() {
    std::cout << "thread start" << std::endl;
    sleep(5);
  });
*/
  ioloop.Init();
  ioloop.start();
/*
  if (tr.joinable()) {
    tr.join();
  }
  */
}
