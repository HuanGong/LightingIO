#ifndef _NET_HTTP_CONSTANTS_H_H
#define _NET_HTTP_CONSTANTS_H_H

#include <string>

namespace net {

struct HttpConstant {
  static const std::string kCRCN;
  static const std::string kBlankSpace;
  static const std::string kConnection;
  static const std::string kContentType;
  static const std::string kContentLength;
  static const std::string kBadRequest;

  static const char* StatusCodeCStr(int32_t code);
};

} //end net::HttpConstant
#endif
