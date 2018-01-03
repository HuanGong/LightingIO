
#include "http_message.h"
#include "base/base_constants.h"

namespace net {

HttpMessage::HttpMessage(IODirectionType t)
  : ProtocolMessage(t, "line") {
}

HttpMessage::~HttpMessage() {

}
std::string& HttpMessage::MutableBody() {
  return body_;
}
const std::string& HttpMessage::Body() const {
  return body_;
}

KeyValMap& HttpMessage::MutableHeaders() {
  return headers_;
}
const KeyValMap& HttpMessage::Headers() const {
  return headers_;
}

KeyValMap& HttpMessage::MutableParams() {
  return params_;
}

const std::string& HttpMessage::GetUrlParam(const char* key) {
  const std::string str_key(key);
  return GetUrlParam(str_key);
}
const std::string& HttpMessage::GetUrlParam(const std::string& key) {
  const auto& val = params_.find(key);
  return val != params_.end() ? val->second : base::kNullString;
}

const std::string HttpMessage::MethodAsString() const {
  switch(method_) {
    case HTTP_GET:
      return "GET";
    case HTTP_POST:
      return "POST";
    case HTTP_HEAD:
      return "HEAD";
    case HTTP_PUT:
      return "PUT";
    case HTTP_DELETE:
      return "DELETE";
    case HTTP_OPTIONS:
      return "OPTIONS";
    case HTTP_TRACE:
      return "TRACE";
    case HTTP_CONNECT:
      return "CONNECT";
    case HTTP_PATCH:
      return "PATCH";
    default:
      return "";
  }
  return "";
}


};//end namespace net
