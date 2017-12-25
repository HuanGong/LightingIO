#ifndef NET_URL_STRING_HELPER_H_H
#define NET_URL_STRING_HELPER_H_H

#include <string>
#include <inttypes.h>

namespace net {
namespace url {

typedef struct {
  uint16_t port;
  std::string ip;
  std::string scheme;
} SchemeIpPort;

/* only for scheme://xx.xx.xx.xx:port format */
uint16_t GetPort(const std::string scheme_ip_port);
std::string GetIP(const std::string scheme_ip_port);
std::string GetScheme(const std::string scheme_ip_port);
bool ParseSchemeIpPortString(const std::string, SchemeIpPort& out);



}}
#endif
