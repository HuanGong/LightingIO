#ifndef NET_URL_REQUEST_H_H
#define NET_URL_REQUEST_H_H

#include "base/base_micro.h"

#include <map>
#include <set>
#include <string>
#include <memory>
#include <unordered_map>
#include <sstream>

namespace net {

class HttpResponse;

//typedef std::unique_prt<std::string> UniqueContent;
typedef std::map<std::string, std::string> KeyValMap;
typedef std::unique_ptr<HttpResponse> UniqueResponse;

enum RequestType {
  INCOMING_REQ = 0,
  OUTGOING_REQ = 1
};

enum HttpMethod {
	HTTP_GET     = 1 << 0,
	HTTP_POST    = 1 << 1,
	HTTP_HEAD    = 1 << 2,
	HTTP_PUT     = 1 << 3,
	HTTP_DELETE  = 1 << 4,
	HTTP_OPTIONS = 1 << 5,
	HTTP_TRACE   = 1 << 6,
	HTTP_CONNECT = 1 << 7,
	HTTP_PATCH   = 1 << 8
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
  void SetContentBody(const char* body, int32_t len);

  // method
  const HttpMethod& ReqeustMethod();
  const std::string GetMethodAsString() const;
  void SetRequestMethod(const HttpMethod method);

  // request url
  const std::string& RequestURL();
  void SetRequestURL(std::string url);

  // Reponse send to clients or Get From server according to RequestType
  // friend class ResponseSender ; CoroResponseSender
  const HttpResponse& Response();
  void SetReplyResponse(UniqueResponse response);

  const std::string& GetHost();
  void SetHost(const char* host);
  void SetHost(const std::string& host);

  const std::string& GetURLPath();
  void SetURLPath(const char* path);
  void SetURLPath(const std::string& path);

  bool IsOutGoingRequest();
  void Dump2Sstream(std::ostream& os) const;
private:
  RequestType type_;

  std::string host_;
  HttpMethod method_;

  std::string request_url_;
  std::string url_path_;
  KeyValMap params_;
  KeyValMap headers_;

  std::string content_;

  UniqueResponse response_;
  DISALLOW_COPY_AND_ASSIGN(HttpUrlRequest);
};

}
#endif
