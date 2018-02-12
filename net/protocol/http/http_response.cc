
#include <sstream>
#include "glog/logging.h"
#include "http_response.h"
#include "http_constants.h"
#include "base/base_constants.h"
#include "base/string_utils/string.hpp"

namespace net {

HttpResponse::HttpResponse(IODirectionType t)
  : ProtocolMessage(t, "http"),
  keepalive_(false),
  http_major_(1),
  http_minor_(1) {
}

HttpResponse::~HttpResponse() {
}

std::string& HttpResponse::MutableBody() {
  return body_;
}
const std::string& HttpResponse::Body() const {
  return body_;
}

KeyValMap& HttpResponse::MutableHeaders() {
  return headers_;
}

const KeyValMap& HttpResponse::Headers() const {
  return headers_;
}

bool HttpResponse::HasHeaderField(const std::string field) const {
  return headers_.find(field) != headers_.end();
}

void HttpResponse::InsertHeader(const char* k, const char* v) {
  headers_.insert(std::make_pair(k, v));
}

void HttpResponse::InsertHeader(const std::string& k, const std::string& v) {
  headers_.insert(std::make_pair(k, v));
}

const std::string& HttpResponse::GetHeader(const std::string& field) const {
  auto iter = headers_.find(field);
  if (iter != headers_.end()) {
    return iter->second;
  }
  return base::kNullString;
}

const std::string HttpResponse::MessageDebug() {
  std::ostringstream oss;
  oss << "{\"type\": \"" << DirectionTypeStr() << "\""
      << ", \"http_major\": " << (int)http_major_
      << ", \"http_minor\": " << (int)http_minor_
      << ", \"status_code\": " << (int)status_code_;

  for (const auto& pair : headers_) {
    oss << ", \"header." << pair.first << "\": \"" << pair.second << "\"";
  }
  oss << ", \"keep_alive\": " << keepalive_
      << ", \"body\": \"" << body_ << "\""
      << "}";
  return oss.str();
}

const char* HttpResponse::DirectionTypeStr() {
  switch(MessageDirection()) {
    case IODirectionType::kInRequest:
      return "HttpResponse In";
    case IODirectionType::kOutRequest:
      return "HttpResponse OUT";
    case IODirectionType::kOutResponse:
      return "HttpResponse OUT";
    case IODirectionType::kInReponse:
      return "HttpResponse IN";
    default:
      return "kUnknownType";
  }
  return "kUnknownType";
}

bool HttpResponse::IsKeepAlive() const {
  return keepalive_;
}

void HttpResponse::SetKeepAlive(bool alive) {
  keepalive_ = alive;
}

uint16_t HttpResponse::ResponseCode() const {
  return status_code_;
}

void HttpResponse::SetResponseCode(uint16_t code) {
  status_code_ = code;
}

std::string HttpResponse::StatusCodeInfo() const {
  const char* desc = http_status_desc(status_code_);
  return desc;
}

};//end namespace net
