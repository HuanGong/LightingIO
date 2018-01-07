
#ifndef _NET_HTTP_RESPONSE_MESSAGE_H
#define _NET_HTTP_RESPONSE_MESSAGE_H

#include <map>
#include <sstream>
#include "protocol/proto_message.h"
#include "http_parser/http_parser.h"

namespace net {

class HttpResponse;

typedef std::map<std::string, std::string> KeyValMap;
typedef std::shared_ptr<HttpResponse> RefHttpResponse;

class HttpResponse : public ProtocolMessage {
public:
  HttpResponse(IODirectionType t);
  ~HttpResponse();

  std::string& MutableBody();
  const std::string& Body() const;

  KeyValMap& MutableHeaders();

  const KeyValMap& Headers() const;
  bool HasHeaderField(const std::string) const;
  void InsertHeader(const std::string&, const std::string&);

  bool IsKeepAlive() const;
  void SetKeepAlive(bool alive);

  const std::string MessageDebug();
  bool ToResponseRawData(std::ostringstream& oss) const;

  uint16_t ResponseCode() const;
  void SetResponseCode(uint16_t code);
  std::string StatusCodeInfo() const;
private:
  friend class HttpProtoService;
  friend class ResParseContext;

  const char* DirectionTypeStr();

  bool keepalive_;
  uint8_t http_major_;
  uint8_t http_minor_;
  uint16_t status_code_;

  std::string body_;
  KeyValMap headers_;
};


}
#endif
