
#include "inet_address.h"
#include "net_endian.h"
#include <strings.h>
#include "glog/logging.h"

namespace net {

InetAddress::InetAddress(uint16_t port) {
  bzero(&addr_in_, sizeof addr_in_);

  addr_in_.sin_family = AF_INET;
  in_addr_t ip = INADDR_ANY;//0.0.0.0
  addr_in_.sin_addr.s_addr = endian::HostToNetwork32(ip);
  addr_in_.sin_port = endian::HostToNetwork16(port);
}

// @c ip should be "1.2.3.4"
InetAddress::InetAddress(std::string ip, uint16_t port) {

  bzero(&addr_in_, sizeof addr_in_);
  socketutils::FromIpPort(ip.c_str(), port, &addr_in_);
}

InetAddress::InetAddress(const struct sockaddr_in& addr)
  : addr_in_(addr) {
}

//static
InetAddress InetAddress::FromSocketFd(int fd) {
  struct sockaddr_in addr = socketutils::GetLocalAddrIn(fd);
  return InetAddress(addr);
}

struct sockaddr* InetAddress::AsSocketAddr() {
  return socketutils::sockaddr_cast(&addr_in_);
}

inline uint16_t InetAddress::PortAsUInt() {
  return endian::NetworkToHost16(addr_in_.sin_port);
}

sa_family_t InetAddress::SocketFamily() {
  LOG(ERROR) << "AF_INET" << AF_INET << " sin_family:" << addr_in_.sin_family;
  return addr_in_.sin_family;
}
std::string InetAddress::IpAsString() {
  return socketutils::SocketAddr2Ip(socketutils::sockaddr_cast(&addr_in_));
}
std::string InetAddress::PortAsString() {
  return std::to_string(PortAsUInt());
}

std::string InetAddress::IpPortAsString() const {
  return socketutils::SocketAddr2IpPort(socketutils::sockaddr_cast(&addr_in_));
}

uint32_t InetAddress::NetworkEndianIp() {
  return addr_in_.sin_addr.s_addr;
}
uint16_t InetAddress::NetworkEndianPort() {
  return addr_in_.sin_port;
}

}
