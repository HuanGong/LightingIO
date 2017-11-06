#include <errno.h>
#include <fcntl.h>
#include <stdio.h>  // snprintf
#include <strings.h>  // bzero
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <netinet/in.h>
#include <assert.h>

#include "net_endian.h"
#include "glog/logging.h"

#include <sys/types.h>
#include <arpa/inet.h>

namespace net {

namespace socketutils {

#define SocketFd int

template<typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}


struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr) {
  return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) {
  return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr) {
  return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) {
  return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr)  {
  return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr));
}

SocketFd CreateNonBlockingFd(sa_family_t family) {
  int sockfd = ::socket(family,
                        SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (sockfd < 0) {
    LOG(ERROR) << " CreateNonBlockingFd call ::socket Failed";
    abort();
  }
  return sockfd;
}

void BindSocketFd(SocketFd sockfd, const struct sockaddr* addr) {
  int ret = ::bind(sockfd,
                   addr,
                   static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG(ERROR) << " BindSocketFd call ::bind Failed";
    abort();
  }
}

SocketFd ScoketConnect(SocketFd fd, const struct sockaddr* addr) {
  return ::connect(fd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

void ListenSocket(SocketFd sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    LOG(ERROR) << "socketutils::ListenSocket Error";
  }
}

SocketFd AcceptSocket(SocketFd sockfd, struct sockaddr_in6* addr) {
  socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
  int connfd = ::accept4(sockfd, sockaddr_cast(addr),
                         &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd < 0) {
    int savedErrno = errno;
    LOG(ERROR) << "SocketUtils::AcceptSocket Error";
    switch (savedErrno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO: // ???
      case EPERM:
      case EMFILE: // per-process lmit of open file desctiptor ???
        // expected errors
        errno = savedErrno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        LOG(ERROR) << "unexpected error of ::accept " << savedErrno;
        break;
      default:
        LOG(ERROR) << "unknown error of ::accept " << savedErrno;
        break;
    }
  }
  return connfd;
}

ssize_t Read(SocketFd sockfd, void* buf, size_t count) {
  return ::read(sockfd, buf, count);
}

ssize_t Write(SocketFd sockfd, const void* buf, size_t count) {
  return ::write(sockfd, buf, count);
}

ssize_t ReadV(SocketFd sockfd, const struct iovec* iov, int iovcnt) {
  return ::readv(sockfd, iov, iovcnt);
}

void CloseSocket(SocketFd sockfd) {
  if (::close(sockfd) < 0) {
    LOG(ERROR) << "socketutils::CloseSocket Failed";
  }
}

void ShutdownWrite(SocketFd sockfd) {
  if (::shutdown(sockfd, SHUT_WR) < 0) {
    LOG(ERROR) << "socketutils::ShutdownWrite Failed";
  }
}

std::string SocketAddr2Ip(const struct sockaddr* addr) {
  char buf[64] = "";
  if (addr->sa_family == AF_INET) {

    assert(64 >= INET_ADDRSTRLEN);
    const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);

    ::inet_ntop(AF_INET,
                &addr4->sin_addr,
                buf,
                static_cast<socklen_t>(sizeof buf));

  } else if (addr->sa_family == AF_INET6) {

    assert(64 >= INET6_ADDRSTRLEN);
    const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);

    ::inet_ntop(AF_INET6,
                &addr6->sin6_addr,
                buf,
                static_cast<socklen_t>(sizeof buf));
  }
  return buf;
}

std::string SocketAddr2IpPort(const struct sockaddr* addr) {
  std::string ip_port = SocketAddr2Ip(addr);

  const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);

  uint16_t port = endian::NetworkToHost16(addr4->sin_port);

  ip_port += (":" + std::to_string(port));
  return ip_port;
}

void FromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr) {

  addr->sin_family = AF_INET;
  addr->sin_port = endian::HostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
    LOG(ERROR) << " ::inet_pton call ERROR";
  }
}

void FromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr) {
  addr->sin6_family = AF_INET6;
  addr->sin6_port = endian::HostToNetwork16(port);
  if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
    LOG(ERROR) << " FromIpPort call inet_pton ERROR";
  }
}

int GetSocketError(SocketFd sockfd) {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);

  if (::getsockopt(sockfd,
                   SOL_SOCKET,
                   SO_ERROR,
                   &optval,
                   &optlen) < 0) {
    return errno;
  }
  return optval;
}

struct sockaddr_in6 GetLocalAddr(int sockfd) {
  struct sockaddr_in6 localaddr;
  bzero(&localaddr, sizeof localaddr);

  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
    LOG(ERROR) << "socketutils::GetLocalAddr Call getsockname ERROR";
  }
  return localaddr;
}

struct sockaddr_in6 GetPeerAddr(int sockfd) {
  struct sockaddr_in6 peeraddr;
  bzero(&peeraddr, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
    LOG(ERROR) << "socketutils::GetPeerAddr Failed";
  }
  return peeraddr;
}

bool IsSelfConnect(int sockfd) {
  struct sockaddr_in6 localaddr = GetLocalAddr(sockfd);
  struct sockaddr_in6 peeraddr = GetPeerAddr(sockfd);
  if (localaddr.sin6_family == AF_INET) {

    const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
    const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port &&
           laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;

  } else if (localaddr.sin6_family == AF_INET6) {

    return localaddr.sin6_port == peeraddr.sin6_port &&
           memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
  }

  return false;
}


}} //end net::socketutils
