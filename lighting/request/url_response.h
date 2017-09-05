#ifndef NET_URL_RESPONSE_H_H
#define NET_URL_RESPONSE_H_H

#include <string>
#include <map>
#include <set>
#include <memory>

namespace net {

typedef std::map<std::string, std::string> HeadersMap;

class HttpResponse {
public:
  HttpResponse(int32_t code);
  ~HttpResponse();

  void SetVersion(char major, char minor);

  const int32_t& Code();
  void SetCode(int32_t code);

  const std::string CodeMessage();
  void SetCodeMessage(std::string message);

  const std::string& ResponseBody();
  void SetResponseBody(std::string& response);

  const HeadersMap& Headers();
  HeadersMap& MutableHeaders();
  bool HasHeader(const std::string&);
  void DelHeader(const std::string&);
  HttpResponse& AddHeader(const std::string&, const std::string&);
  void SwapHeaders(HeadersMap& headers);
  const std::string& GetHeader(const std::string& header);

  std::string ToString();
private:
  char version_major_;
  char version_minor_;

  int32_t reponse_code_;
  std::string code_message_;

  HeadersMap headers_;

  std::string content_;
};

} //end net
#endif
