
#include "url_request.h"

//class HttpResponse;
namespace net {

UrlRequest::UrlRequest(RequestType type);
UrlRequest::~UrlRequest();

  // header
inline const HeadersMap& UrlRequest::Headers() {
  return headers_;
}

inline HeadersMap& UrlRequest::MutableHeaders() {
  return headers_;
}

inline bool UrlRequest::HasHeader(const std::string& key) {
  return headers_.find(key) != HeadersMap::end();
}

void UrlRequest::DelHeader(const std::string& key) {
  headers_.remove(key);
}

void UrlRequest::AddHeader(const std::string& key, const std::string& val) {
  headers_[key] = val;
}

void UrlRequest::SwapHeaders(HeadersMap& headers) {
  headers_.swap(headers);
}

// content
const std::string& UrlRequest::ContentBody() {
  return content_;
}

void UrlRequest::SetContentBody(std::string& body) {
  content_ = body;
}

// method
const std::string& UrlRequest::ReqeustMethod() {
  return method_;
}

void UrlRequest::SetRequestMethod(const std::string& method) {
  method_ = method;
}

const std::string& UrlRequest::RequestURL() {
  return reqeust_url_;
}

void UrlRequest::SetRequestURL(std::string url) {
  request_url = url;
}

// Reponse send to clients or Get From server according to RequestType
// friend class ResponseSender ; CoroResponseSender
const HttpResponse& UrlRequest::Response() {
  return response_;
}

void UrlRequest::SetReplyResponse(UniqueResponse response) {
  response = std::move(response);
}

} //end net
