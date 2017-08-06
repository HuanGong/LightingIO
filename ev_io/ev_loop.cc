
#include "ev_loop.h"


namespace IO {

EvIOLoop::EvIOLoop() :
  ev_base_(NULL) {
  Init();
}

EvIOLoop::~EvIOLoop() {

}

void EvIOLoop::start() {

}

void EvIOLoop::stop() {

}

int EvIOLoop::status() {
  return 0;
}
void EvIOLoop::pause() {

}
void EvIOLoop::resume() {

}

}
