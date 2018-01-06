
#include <sstream>
#include "http_request.h"
#include "http_constants.h"
#include "base/base_constants.h"

#include "base/string_utils/string.hpp"

namespace net {

HttpRequest::HttpRequest(IODirectionType t)
  : ProtocolMessage(t, "http"),
  keepalive_(true),
  http_major_(1),
  http_minor_(1),
  url_param_parsed_(false) {

}

HttpRequest::~HttpRequest() {

}

const std::string& HttpRequest::RequestUrl() const {
  return url_;
}

std::string& HttpRequest::MutableBody() {
  return body_;
}
const std::string& HttpRequest::Body() const {
  return body_;
}

KeyValMap& HttpRequest::MutableHeaders() {
  return headers_;
}
const KeyValMap& HttpRequest::Headers() const {
  return headers_;
}

bool HttpRequest::HasHeaderField(const std::string field) const {
  return headers_.find(field) != headers_.end();
}

void HttpRequest::InsertHeader(const std::string& k, const std::string& v) {
  headers_.insert(std::make_pair(k, v));
}

KeyValMap& HttpRequest::MutableParams() {
  return params_;
}

void HttpRequest::ParseUrlToParams() {
  url_param_parsed_ = true;
}

const std::string& HttpRequest::GetUrlParam(const char* key) {
  const std::string str_key(key);
  return GetUrlParam(str_key);
}
const std::string& HttpRequest::GetUrlParam(const std::string& key) {
  if (params_.empty() && !url_param_parsed_) {
    ParseUrlToParams();
  }
  const auto& val = params_.find(key);
  return val != params_.end() ? val->second : base::kNullString;
}

const std::string& HttpRequest::Method() const {
  return method_;
}

const bool HttpRequest::ToRequestRawData(std::ostringstream& oss) {

  //request line
  oss << method_ << HttpConstant::kBlankSpace << url_ << HttpConstant::kBlankSpace
    << "HTTP/" << http_major_ << "." << http_minor_ << HttpConstant::kCRCN;

  // header part
  for (const auto& header : Headers()) {
    oss << header.first << ":" << header.second << HttpConstant::kCRCN;
  }

  if (!HasHeaderField(HttpConstant::kConnection)) {
    oss << "Connection: " << (keepalive_ ? "Keep-Alive" : " Close")  << HttpConstant::kCRCN;
  }

  if (!HasHeaderField(HttpConstant::kContentLength)) {
    oss << HttpConstant::kContentLength << ":" << body_.size() << HttpConstant::kCRCN;
  }

  if (!HasHeaderField(HttpConstant::kContentType)) {
    oss << HttpConstant::kContentType << ":" << "text/html" << HttpConstant::kCRCN;
  }
  //body
  oss << HttpConstant::kCRCN << body_;

  return true;
}

const std::string HttpRequest::MessageDebug() {
  std::ostringstream oss;
  oss << "{\"type\": \"" << DirectionTypeStr() << "\""
      << ", \"http_major\": " << (int)http_major_
      << ", \"http_minor\": " << (int)http_minor_
      << ", \"method\": \"" << Method() << "\""
      << ", \"request_url\": \"" << url_ << "\"";

  for (const auto& pair : headers_) {
    oss << ", \"header." << pair.first << "\": \"" << pair.second << "\"";
  }
  oss << ", \"keep_alive\": " << keepalive_
      << ", \"body\": \"" << body_ << "\""
      << "}";
  return oss.str();
}

void HttpRequest::SetMethod(const std::string method) {
  method_ = str::to_upper_copy(method);
}

const char* HttpRequest::DirectionTypeStr() {
  switch(MessageDirection()) {
    case IODirectionType::kInRequest:
      return "HttpRequest In";
    case IODirectionType::kOutRequest:
      return "HttpRequest OUT";
    case IODirectionType::kOutResponse:
      return "HttpResponse OUT";
    case IODirectionType::kInReponse:
      return "HttpResponse IN";
    default:
      return "kUnknownType";
  }
  return "kUnknownType";
}

bool HttpRequest::IsKeepAlive() const {
  return keepalive_;
}

void HttpRequest::SetKeepAlive(bool alive) {
  keepalive_ = alive;
}

};//end namespace net
