#ifndef IO_EV_LOOP_H_H
#define IO_EV_LOOP_H_H

#inlcude "event.h"

namespace IO {

class EvIOLoop {
  public:
    EvIOLoop();
    ~EvIOLoop();

    void start();
    void stop();
    int status();
    void pause();
    void resume();

  private:
    Init();

  private:
    event_base* ev_base_;
};

} //endnamespace
#endif
