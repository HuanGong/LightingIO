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
typedef std::map<std::string, std::string> KeyValMap;
typedef std::unique_ptr<HttpResponse> UniqueResponse;

enum RequestType {
  INCOMING_REQ = 0,
  OUTGOING_REQ = 1
};

class HttpUrlRequest {
public:
  HttpUrlRequest(RequestType type);
  ~HttpUrlRequest();

  // header
  const KeyValMap& Headers();
  KeyValMap& MutableHeaders();
  bool HasHeader(const std::string&);
  void DelHeader(const std::string&);
  void AddHeader(const std::string&, const std::string&);
  void SwapHeaders(KeyValMap& headers);
  const std::string& GetHeader(const std::string& header);

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

  bool IsOutGoingRequest();
private:
  RequestType type_;

  std::string method_;
  std::string request_url_;

  KeyValMap params_;
  KeyValMap headers_;

  std::string content_;

  UniqueResponse response_;
  DISALLOW_COPY_AND_ASSIGN(HttpUrlRequest);
};

}
#endif
