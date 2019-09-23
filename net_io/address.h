#ifndef _NET_INET_ADDRESS_H_
#define _NET_INET_ADDRESS_H_

#include <inttypes.h>
#include "socket_utils.h"

namespace lt {
namespace net {

class SocketAddr {
public:
  static SocketAddr FromSocketFd(int fd);
public:
  SocketAddr(uint16_t port);
  explicit SocketAddr(const struct sockaddr_in&);
  // @c ip should be "1.2.3.4"
  SocketAddr(const std::string& ip, const uint16_t port);

  uint16_t Port() const;
  std::string Ip() const;
  std::string IpPort() const;

  sa_family_t Family() const;
  uint32_t NetworkEndianIp() const;
  uint16_t NetworkEndianPort() const;

  const struct sockaddr* AsSocketAddr() const;
  const struct sockaddr_in* SockAddrIn() const {return &addr_in_;}
private:
  struct sockaddr_in addr_in_;
};

}};
#endif
