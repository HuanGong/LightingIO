#include "../event_pump.h"
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <iostream>
#include <sys/epoll.h>
#include "../fd_event.h"

int main() {
  base::EventPump loop;

  int fd[2];
  if (0 == pipe2(fd, O_CLOEXEC | O_NONBLOCK)) {
    std::cout << "Create O_NONBLOCK pipe ok" << std::endl;
  }
  int read_fd = fd[0];
  int write_fd = fd[1];
  std::cout << "read fd" << read_fd << " write fd:" << write_fd << std::endl;

  base::FdEvent fd_ev(read_fd, EPOLLIN);
  fd_ev.set_read_callback([&]() {
    int value = 0;
    int count = read(fd_ev.fd(), (char*)&value, sizeof(int));
    std::cout << "Get " << count << " bytes data, Value:" << value << std::endl;
  });
  //int FdEvent fd_ev(1, EPOLLOUT);
  std::thread thread([&](){
    int times = 10;
    while(times--) {
      sleep(1);
      int count = write(write_fd, &times, sizeof(times));
      std::cout << "write " << count << " bytes data to fd" << std::endl;
    }
  });

  loop.InstallFdEvent(&fd_ev);
  loop.Run();

  return 0;
}
