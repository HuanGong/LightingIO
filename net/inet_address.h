#ifndef _NET_INET_ADDRESS_H_
#define _NET_INET_ADDRESS_H_

#include <inttypes.h>
#include "socket_utils.h"

/* Thanks for Chensuo's muduo code, give me a good reference for this,
 * but still not good enough*/

namespace net {

class InetAddress {
public:
  InetAddress(uint16_t port);
  // @c ip should be "1.2.3.4"
  InetAddress(std::string ip, uint16_t port);
  explicit InetAddress(const struct sockaddr_in& addr);

  static InetAddress FromSocketFd(int fd);

  uint16_t PortAsUInt();
  sa_family_t SocketFamily();
  std::string IpAsString();
  std::string PortAsString();
  std::string IpPortAsString() const;

  uint32_t NetworkEndianIp();
  uint16_t NetworkEndianPort();

  struct sockaddr_in* SockAddrIn() { return &addr_in_;}
  struct sockaddr* AsSocketAddr();
private:
  struct sockaddr_in addr_in_;
};

};

#endif
