#ifndef LIGHTING_NET_SOCKET_FD_H
#define LIGHTING_NET_SOCKET_FD_H

//forward declare, it is in <netinet/tcp.h>
struct tcp_info;

namespace net {

class SocketFd {
  public:
    SocketFd(int fd);
    ~SocketFd();

    int fd() const { return socket_fd_; }

    void
  private:

    int socket_fd_;
};

}
#endif
