#ifndef NET_URL_STRING_HELPER_H_H
#define NET_URL_STRING_HELPER_H_H

#include <string>
#include <inttypes.h>

namespace net {
namespace url {

typedef struct {
  uint16_t port;
  std::string host;
  std::string host_ip;
  std::string protocol;
} SchemeIpPort;

/* only for scheme://xx.xx.xx.xx:port format */
uint16_t GetPort(const std::string uri);
std::string GetIP(const std::string uri);
std::string GetScheme(const std::string uri);
bool HostResolve(const std::string host, std::string& host_ip);
bool ParseSchemeIpPortString(const std::string, SchemeIpPort& out);



}}
#endif
