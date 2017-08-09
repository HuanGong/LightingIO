


#include "ev_loop.h"
#include "cstdlib"
#include "unistd.h"

#include <iostream>

#include "ev_task.h"
#include "ctime"
#include <time.h>

//int gettimeofday(struct timeval *tv, struct timezone *tz);
long long gettime() {

  //struct timeval tv;
  //gettimeofday(&tv, NULL);
  //return tv.tv_usec/1000 + tv.tv_sec*1000;
  //1s 1000 ms -> 1000000us -> 1000,000,000ns
  struct timespec tpend;
  clock_gettime(CLOCK_MONOTONIC, &tpend);
  return (tpend.tv_sec*1000000000)+(tpend.tv_nsec);
}

int main() {

  IO::EventLoop loop("task loop");
  loop.Start();


  std::cout << "loop end; join number:" << x << std::endl;
  return 0;
}
