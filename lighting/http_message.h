#ifndef HTTP_MESSAGE_H_
#define HTTP_MESSAGE_H_

#include <memory>
#include <iostream>
#include <unordered_map>

#include "protocol_message.h"

enum HttpMethod {
  HTTP_GET = 0,
  HTTP_POST = 1
};

enum HttpMsgType {
  kIncomingRequest,
  kOutgoingRequest,
  kIncomingResponse,
  kOutgoingResponse,
  kOneway,
};

typedef std::unordered_map<std::string, std::string> HeaderMap;

namespace net {
class HttpMessage : public ProtocolMessage {
public:
  typedef std::unique_ptr<HttpMessage> HttpMessageUPtr
  HttpMessage(bool is_comming_msg);
  ~HttpMessage();

  bool is_incomming_msg
  bool DumpToString(std::string* out);
  bool DecodeContent() {};
  bool EncodeContent() {};

  const int& Code() { return msg_code_;}
  const HeaderMap& Headers() {return headers_;};
  const std::string& Cookies() {return cookies_;}
  const std::string& UserAgent() {return user_agent_;}
  const std::string& ContentBody() {return body_content_;}

private:
  std::string host_;
  HttpMethod method_;
  HeaderMap headers_;
  std::string body_content_;

  std::string cookies_;
  std::string user_agent_;
  //incomming message
  bool incomming_msg_;
  //out message
  int msg_code_;
};

}//end net
#endif
