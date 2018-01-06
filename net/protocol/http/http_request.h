
#ifndef _NET_HTTP_PROTO_MESSAGE_H
#define _NET_HTTP_PROTO_MESSAGE_H

#include <map>
#include <sstream>
#include "protocol/proto_message.h"
#include "http_parser/http_parser.h"

namespace net {

class HttpRequest;

typedef std::map<std::string, std::string> KeyValMap;
typedef std::shared_ptr<HttpRequest> RefHttpRequest;

class HttpRequest : public ProtocolMessage {
public:
  HttpRequest(IODirectionType t);
  ~HttpRequest();

  std::string& MutableBody();
  const std::string& Body() const;

  void SetMethod(const std::string);
  const std::string& Method() const;

  const std::string& RequestUrl() const;

  KeyValMap& MutableHeaders();
  const KeyValMap& Headers() const;
  bool HasHeaderField(const std::string) const;
  void InsertHeader(const std::string&, const std::string&);

  KeyValMap& MutableParams();
  const std::string& GetUrlParam(const char*);
  const std::string& GetUrlParam(const std::string&);

  bool IsKeepAlive() const;
  void SetKeepAlive(bool alive);

  const std::string MessageDebug();
  const bool ToRequestRawData(std::ostringstream& oss);
private:
  void ParseUrlToParams();
  const char* DirectionTypeStr();
private:
  friend class HttpProtoService;

  bool keepalive_;

  std::string method_;
  std::string url_;
  std::string body_;

  uint8_t http_major_;
  uint8_t http_minor_;

  KeyValMap headers_;

  KeyValMap params_;
  /*indicate url has parsed to params_*/
  bool url_param_parsed_;
};


}
#endif
