
#include <string>
#include <inttypes.h>
#include "url_string_utils.h"

namespace net {
namespace url {

static std::string ip_port_separator(":");
static std::string scheme_ip_separator("://");
/* only for scheme://xx.xx.xx.xx:port format */
std::string GetIP(const std::string scheme_ip_port) {
  uint32_t pos = scheme_ip_port.find(scheme_ip_separator);
  uint32_t port_pos = scheme_ip_port.find(ip_port_separator, pos + scheme_ip_separator.size());
  if (pos == std::string::npos || port_pos == std::string::npos) {
    return "";
  }
  uint32_t ip_start = pos + scheme_ip_separator.size();
  return std::string(scheme_ip_port.begin() + ip_start,
                     scheme_ip_port.begin() + port_pos);
}
uint16_t GetPort(const std::string scheme_ip_port) {
  uint32_t pos = scheme_ip_port.find(scheme_ip_separator);
  uint32_t port_pos = scheme_ip_port.find(ip_port_separator, pos + scheme_ip_separator.size());
  if (pos == std::string::npos || port_pos == std::string::npos) {
    return 0;
  }
  uint32_t port_start = port_pos + ip_port_separator.size();
  std::string port_str = std::string(scheme_ip_port.begin() + port_start,
                                     scheme_ip_port.end());
  return std::atoi(port_str.c_str());
}

std::string GetScheme(const std::string scheme_ip_port) {
  uint32_t pos = scheme_ip_port.find(scheme_ip_separator);
  uint32_t port_pos = scheme_ip_port.find(ip_port_separator, pos + scheme_ip_separator.size());
  if (pos == std::string::npos || port_pos == std::string::npos) {
    return "";
  }
  return std::string(scheme_ip_port.begin(), scheme_ip_port.begin() + pos);
}

bool ParseSchemeIpPortString(const std::string uri, SchemeIpPort& out) {
  uint32_t pos = uri.find(scheme_ip_separator);
  uint32_t port_pos = uri.find(ip_port_separator, pos + scheme_ip_separator.size());
  if (pos == std::string::npos || port_pos == std::string::npos) {
    return false;
  }
  out.scheme = std::string(uri.begin(), uri.begin() + pos);
  uint32_t ip_start = pos + scheme_ip_separator.size();

  out.ip = std::string(uri.begin() + ip_start,
                       uri.begin() + port_pos);

  uint32_t port_start = port_pos + ip_port_separator.size();
  std::string port_str = std::string(uri.begin() + port_start,
                                     uri.end());
  out.port = std::atoi(port_str.c_str());
  return (!out.scheme.empty()) && (!out.ip.empty()) && (out.port != 0);
}

}}
