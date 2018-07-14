#ifndef SIGNAL_H
#define SIGNAL_H

#include <functional>
#include <sys/types.h>
#include <signal.h>

namespace base {

// see https://github.com/yedf/handy/blob/master/handy/daemon.cc
struct Signal {
    static void signal(int sig, const std::function<void()>& handler);
};

} //namespace

#endif // SIGNAL_H
