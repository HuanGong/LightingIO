#include "url_response.h"
#include "base/base_constants.h"

namespace net {

HttpResponse::HttpResponse(int32_t code)
  : reponse_code_(code),
    version_major_(1),
    version_minor_(1) {
}

HttpResponse::~HttpResponse() {

}

void HttpResponse::SetVersion(char major, char minor) {
  version_major_ = major;
  version_minor_ = minor;
}

const int32_t& HttpResponse::Code() const {
  return reponse_code_;
}

void HttpResponse::SetCode(int32_t code) {
  reponse_code_ = code;
}

const std::string HttpResponse::CodeMessage() {
  return code_message_;
}

void HttpResponse::SetCodeMessage(std::string message) {
  code_message_ = message;
}

const std::string& HttpResponse::ResponseBody() const {
  return content_;
}

void HttpResponse::SetResponseBody(std::string& response) {
  content_ = response;
}

const HeadersMap& HttpResponse::Headers() const {
  return headers_;
}

HeadersMap& HttpResponse::MutableHeaders() {
  return headers_;
}

inline bool HttpResponse::HasHeader(const std::string& key) {
  return headers_.find(key) != headers_.end();
}

void HttpResponse::DelHeader(const std::string& key) {
  headers_.erase(key);
}

HttpResponse& HttpResponse::AddHeader(const std::string& key, const std::string& val) {
  headers_[key] = val;
  return *this;
}

void HttpResponse::SwapHeaders(HeadersMap& headers) {
  headers_.swap(headers_);
}

inline const std::string& HttpResponse::GetHeader(const std::string& header) {
  auto iter = headers_.find(header);
  return (iter != headers_.end()) ? iter->second : base::kNullString;
}

} //end namespace
