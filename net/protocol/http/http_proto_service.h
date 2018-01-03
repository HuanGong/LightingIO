#ifndef NET_HTTP_PROTO_SERVICE_H
#define NET_HTTP_PROTO_SERVICE_H

#include "protocol/proto_service.h"
#include "http_parser/http_parser.h"

namespace net {

typedef struct {
  const char* head_field_;
  size_t header_field_len_;
} HeaderField;

class HttpProtoService : public ProtoService {
public:
  HttpProtoService();
  ~HttpProtoService();

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataRecieved(const RefTcpChannel&, IOBuffer*) override;

  //no SharedPtr here, bz of type_cast and don't need guarantee it's lifetime in this contex
  bool DecodeBufferToMessage(IOBuffer* buffer, ProtocolMessage* out_msg);
  bool EncodeMessageToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer);
private:
  http_parser parser_;
  HeaderField wait_value_header_;

  static http_parser_settings parser_settings_;
};

}
#endif
