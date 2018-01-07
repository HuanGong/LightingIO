#ifndef NET_HTTP_PROTO_SERVICE_H
#define NET_HTTP_PROTO_SERVICE_H

#include <vector>
#include <string>
#include "net_callback.h"
#include "parser_context.h"
#include "protocol/proto_service.h"
#include "http_parser/http_parser.h"

namespace net {

typedef struct {
  void reset() {
    status_code.clear();
    current_.reset();
    half_header.first.clear();
    half_header.second.clear();
    last_is_header_value = false;
  }
  std::string status_code;
  bool last_is_header_value;
  std::pair<std::string, std::string> half_header;
  RefHttpRequest current_;
  std::vector<RefHttpRequest> messages_;
} HttpParseContext;

class HttpProtoService : public ProtoService {
public:
  HttpProtoService();
  ~HttpProtoService();

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataRecieved(const RefTcpChannel&, IOBuffer*) override;

  //no SharedPtr here, bz of type_cast and don't need guarantee it's lifetime in this contex
  bool DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg);
  bool EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer);
private:
  //bool EncodeHttpResponse(const HttpResponse*, IOBU* out_buffer);
  bool ParseHttpRequest(const RefTcpChannel&, IOBuffer*);
  bool ParseHttpResponse(const RefTcpChannel&, IOBuffer*);
private:
  ReqParseContext* request_context_;
  ResParseContext* response_context_;

  static http_parser_settings req_parser_settings_;
  static http_parser_settings res_parser_settings_;
};

}
#endif
