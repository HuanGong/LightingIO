
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

inline KeyValMap& HttpUrlRequest::MutableHeaders() {
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

// method
const std::string& HttpUrlRequest::ReqeustMethod() {
  return method_;
}

void HttpUrlRequest::SetRequestMethod(const std::string& method) {
  method_ = method;
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

} //end net
