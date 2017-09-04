#ifndef NET_URL_REQUEST_H_H
#define NET_URL_REQUEST_H_H

#include "base/base_micro.h"

#include <map>
#include <set>
#include <string>
#include <memory>
#include <unordered_map>

namespace net {

class HttpResponse;

//typedef std::unique_prt<std::string> UniqueContent;
typedef std::map<std::string, std::string> HeadersMap;
typedef std::unique_ptr<HttpResponse> UniqueResponse;

enum RequestType {
  INCOMING_REQ = 0,
  OUTGOING_REQ = 1
};

class HttpUrlRequest {
public:
  UrlRequest(RequestType type);
  ~UrlRequest();

  // header
  const HeadersMap& Headers();
  HeadersMap& MutableHeaders();
  void HasHeader(const std::string&);
  void DelHeader(const std::string&);
  void AddHeader(const std::string&, const std::string&);
  void SwapHeaders(HeadersMap& headers);

  // content
  const std::string& ContentBody();
  void SetContentBody(std::string& body);

  // method
  const std::string& ReqeustMethod();
  void SetRequestMethod(const std::string& method);

  // request url
  const std::string& RequestURL();
  void SetRequestURL(std::string url);

  // Reponse send to clients or Get From server according to RequestType
  // friend class ResponseSender ; CoroResponseSender
  const HttpResponse& Response();
  void SetReplyResponse(UniqueResponse response);

private:
  std::string method_;
  std::string reqeust_url_;

  HeadersMap headers_;
  std::string content_;

  UniqueResponse response_;
  DISALLOW_COPY_AND_ASSIGN(HttpUrlRequest);
};

}
#endif
