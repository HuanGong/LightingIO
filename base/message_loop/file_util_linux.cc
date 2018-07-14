#include "file_util_linux.h"

#include <unistd.h>
#include <fcntl.h>

namespace base {

bool CreateLocalNoneBlockingPipe(int fds[2]) {
  return ::pipe2(fds, O_CLOEXEC | O_NONBLOCK) == 0;
}

}

