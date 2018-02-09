#ifndef NET_HTTP_PROTO_SERVICE_H
#define NET_HTTP_PROTO_SERVICE_H

#include <vector>
#include <string>
#include "net_callback.h"
#include "parser_context.h"
#include "protocol/proto_service.h"
#include "http_parser/http_parser.h"

namespace net {
class HttpProtoService;
typedef std::shared_ptr<HttpProtoService> RefHttpProtoService;

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

  const RefProtocolMessage DefaultResponse(const RefProtocolMessage&) override;
  bool CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) override;

  static bool RequestToBuffer(const HttpRequest*, IOBuffer*);
  static bool ResponseToBuffer(const HttpResponse*, IOBuffer*);
private:
  bool ParseHttpRequest(const RefTcpChannel&, IOBuffer*);
  bool ParseHttpResponse(const RefTcpChannel&, IOBuffer*);

private:
  ReqParseContext* request_context_;
  ResParseContext* response_context_;
  bool close_after_finish_send_;
  static http_parser_settings req_parser_settings_;
  static http_parser_settings res_parser_settings_;
};

}
#endif
