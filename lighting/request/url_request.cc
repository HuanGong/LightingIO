
#include "url_request.h"
#include "url_response.h"
#include "base/base_constants.h"

//class HttpResponse;
namespace net {

HttpUrlRequest::HttpUrlRequest(RequestType type) {
  type_ = type;
}
HttpUrlRequest::~HttpUrlRequest() {

}

inline bool HttpUrlRequest::IsOutGoingRequest() {
  return type_ == RequestType::OUTGOING_REQ;
}
  // header
inline const KeyValMap& HttpUrlRequest::Headers() {
  return headers_;
}

KeyValMap& HttpUrlRequest::MutableHeaders() {
  return headers_;
}

inline bool HttpUrlRequest::HasHeader(const std::string& key) {
  return headers_.find(key) != headers_.end();
}

void HttpUrlRequest::DelHeader(const std::string& key) {
  headers_.erase(key);
}

void HttpUrlRequest::AddHeader(const std::string& key, const std::string& val) {
  headers_[key] = val;
}

void HttpUrlRequest::SwapHeaders(KeyValMap& headers) {
  headers_.swap(headers);
}

inline const std::string& HttpUrlRequest::GetHeader(const std::string& header) {
  auto iter = headers_.find(header);
  return (iter != headers_.end()) ? iter->second : base::kNullString;
}

// content
const std::string& HttpUrlRequest::ContentBody() {
  return content_;
}

void HttpUrlRequest::SetContentBody(std::string& body) {
  content_ = body;
}

void HttpUrlRequest::SetContentBody(const char* body, int32_t len) {
  content_.reserve(len);
  content_.assign(body, len);
}

// method
const HttpMethod& HttpUrlRequest::ReqeustMethod() {
  return method_;
}

void HttpUrlRequest::SetRequestMethod(const HttpMethod method) {
  method_ = method;
}

const std::string HttpUrlRequest::GetMethodAsString() const {
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

const std::string& HttpUrlRequest::RequestURL() {
  return request_url_;
}

void HttpUrlRequest::SetRequestURL(std::string url) {
  request_url_ = url;
}

// Reponse send to clients or Get From server according to RequestType
// friend class ResponseSender ; CoroResponseSender
const HttpResponse& HttpUrlRequest::Response() {
  static HttpResponse null_response(0);
  return response_.get() ? *(response_.get()) : null_response;
}

void HttpUrlRequest::SetReplyResponse(UniqueResponse response) {
  response_ = std::move(response);
}

inline const std::string& HttpUrlRequest::GetHost() {
  return host_;
}

void HttpUrlRequest::SetHost(const char* host) {
  host_ = host;
}

void HttpUrlRequest::SetHost(const std::string& host) {
  host_ = host;
}

const std::string& HttpUrlRequest::GetURLPath() {
  return url_path_;
}

void HttpUrlRequest::SetURLPath(const char* path) {
  url_path_ = path;
}

void HttpUrlRequest::SetURLPath(const std::string& path) {
  url_path_ = path;
}

void HttpUrlRequest::Dump2Sstream(std::ostream& os) const {
  os << "Reqeust Type:" << ((type_ == RequestType::INCOMING_REQ) ? "INCOMING_REQ" : "OUTGOING_REQ") << std::endl;
  os << "Host" << host_ << std::endl;
  os << "URL:" << request_url_ << std::endl;
  os << "URL PATH:" << url_path_ << std::endl;
  os << "Method: " << GetMethodAsString() << std::endl;
  for (auto& header : headers_) {
    os << header.first << ": " << header.second << std::endl;
  }
  os << "Body:" << content_ << std::endl;
}

} //end net
