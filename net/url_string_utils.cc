#include <string>
#include <inttypes.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "glog/logging.h"
#include "url_string_utils.h"

namespace net {
namespace url {

static std::string ip_port_separator(":");
static std::string scheme_ip_separator("://");
/* only for scheme://xx.xx.xx.xx:port format */
std::string GetIP(const std::string uri) {
  uint32_t pos = uri.find(scheme_ip_separator);
  uint32_t port_pos = uri.find(ip_port_separator, pos + scheme_ip_separator.size());
  if (pos == std::string::npos || port_pos == std::string::npos) {
    return "";
  }
  uint32_t ip_start = pos + scheme_ip_separator.size();
  return std::string(uri.begin() + ip_start,
                     uri.begin() + port_pos);
}
uint16_t GetPort(const std::string uri) {
  uint32_t pos = uri.find(scheme_ip_separator);
  uint32_t port_pos = uri.find(ip_port_separator, pos + scheme_ip_separator.size());
  if (pos == std::string::npos || port_pos == std::string::npos) {
    return 0;
  }
  uint32_t port_start = port_pos + ip_port_separator.size();
  std::string port_str = std::string(uri.begin() + port_start,
                                     uri.end());
  return std::atoi(port_str.c_str());
}

std::string GetScheme(const std::string uri) {
  uint32_t pos = uri.find(scheme_ip_separator);
  uint32_t port_pos = uri.find(ip_port_separator, pos + scheme_ip_separator.size());
  if (pos == std::string::npos || port_pos == std::string::npos) {
    return "";
  }
  return std::string(uri.begin(), uri.begin() + pos);
}

bool ParseSchemeIpPortString(const std::string uri, SchemeIpPort& out) {
  uint32_t pos = uri.find(scheme_ip_separator);
  uint32_t port_pos = uri.find(ip_port_separator, pos + scheme_ip_separator.size());
  if (pos == std::string::npos || port_pos == std::string::npos) {
    return false;
  }
  out.protocol = std::string(uri.begin(), uri.begin() + pos);
  uint32_t ip_start = pos + scheme_ip_separator.size();

  out.host = std::string(uri.begin() + ip_start, uri.begin() + port_pos);
 
  bool success = HostResolve(out.host, out.host_ip);
  if (false == success) {
    return success;
  }
  
  uint32_t port_start = port_pos + ip_port_separator.size();
  std::string port = std::string(uri.begin() + port_start, uri.end());

  out.port = std::atoi(port.c_str());
  return (!out.protocol.empty()) && (!out.host_ip.empty()) && (out.port != 0);
}

bool HostResolve(const std::string host, std::string& host_ip) {
  int status;
  struct addrinfo hints, *p;
  struct addrinfo *servinfo;
  char ipstr[INET_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family   = AF_UNSPEC;
  hints.ai_flags    = AI_PASSIVE;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(host.c_str(), NULL, &hints, &servinfo)) == -1) {
    //fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return false;
  }
  for (p=servinfo; p!=NULL; p=p->ai_next) {
    struct in_addr  *addr;
    switch(p->ai_family) {
      case AF_INET: {
        struct sockaddr_in *ipv = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv->sin_addr);
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
      } break;
      case AF_INET6: {
        //struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        //addr = (struct in_addr *) &(ipv6->sin6_addr);
      } break;
      default:{
      }break;
    }
  }
  freeaddrinfo(servinfo);
  host_ip = ipstr;
  return !host_ip.empty();
}

}}
