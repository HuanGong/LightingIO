
#ifndef _NET_HTTP_PROTO_MESSAGE_H
#define _NET_HTTP_PROTO_MESSAGE_H

#include <map>
#include "protocol/proto_message.h"

namespace net {

typedef enum {
	HTTP_GET     = 1 << 0,
	HTTP_POST    = 1 << 1,
	HTTP_HEAD    = 1 << 2,
	HTTP_PUT     = 1 << 3,
	HTTP_DELETE  = 1 << 4,
	HTTP_OPTIONS = 1 << 5,
	HTTP_TRACE   = 1 << 6,
	HTTP_CONNECT = 1 << 7,
	HTTP_PATCH   = 1 << 8
}HttpMethod;

typedef std::map<std::string, std::string> KeyValMap;

class HttpMessage : public ProtocolMessage {
public:
  HttpMessage(IODirectionType t);
  ~HttpMessage();

  std::string& MutableBody();
  const std::string& Body() const;

  void SetMethod(const HttpMethod);
  const std::string MethodAsString() const;

  KeyValMap& MutableHeaders();
  const KeyValMap& Headers() const;

  KeyValMap& MutableParams();
  const std::string& GetUrlParam(const char*);
  const std::string& GetUrlParam(const std::string&);
private:
  HttpMethod method_;
  std::string url_;
  std::string body_;

  KeyValMap params_;
  KeyValMap headers_;
};

}
#endif
