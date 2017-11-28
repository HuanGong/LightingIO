#ifndef _NET_INET_ADDRESS_H_
#define _NET_INET_ADDRESS_H_

#include <inttypes.h>
#include "socket_utils.h"

namespace net {

class InetAddress {
public:
  explicit InetAddress(uint16_t port);
  // @c ip should be "1.2.3.4"
  InetAddress(std::string ip, uint16_t port);
  explicit InetAddress(const struct sockaddr_in& addr);

  uint16_t PortAsUInt();
  sa_family_t SocketFamily();
  std::string IpAsString();
  std::string PortAsString();
  std::string IpPortAsString();

  uint32_t NetworkEndianIp();
  uint16_t NetworkEndianPort();

  struct sockaddr_in* SockAddrIn() { return &addr_in_;}
  struct sockaddr* AsSocketAddr();
private:
  struct sockaddr_in addr_in_;
};

};

#endif
