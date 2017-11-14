#include "linux_signal.h"
#include <map>
#include <functional>


namespace base {

std::map<int, std::function<void()>> handlers;

void signal_handler(int sig) {
    handlers[sig]();
}

void Signal::signal(int sig, const std::function<void ()> &handler) {
    handlers[sig] = handler;
    ::signal(sig, signal_handler);
}

}
