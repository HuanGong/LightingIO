


#include "ev_loop.h"

#include <iostream>

int main() {
  IO::EvIOLoop ioloop;
  ioloop.Init();
  ioloop.start();
}
