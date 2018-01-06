#ifndef NET_HTTP_PROTO_SERVICE_H
#define NET_HTTP_PROTO_SERVICE_H

#include <vector>
#include <string>
#include "http_request.h"
#include "net_callback.h"
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

  static int OnHttpRequestBegin(http_parser* parser);
  static int OnUrlParsed(http_parser* parser, const char *url_start, size_t url_len);
  static int OnStatusCodeParsed(http_parser* parser, const char *start, size_t len);
  static int OnHeaderFieldParsed(http_parser* parser, const char *header_start, size_t len);
  static int OnHeaderValueParsed(http_parser* parser, const char *value_start, size_t len);
  static int OnHeaderFinishParsed(http_parser* parser);
  static int OnBodyParsed(http_parser* parser, const char *body_start, size_t len);
  static int OnHttpRequestEnd(http_parser* parser);
  static int OnChunkHeader(http_parser* parser);
  static int OnChunkFinished(http_parser* parser);

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataRecieved(const RefTcpChannel&, IOBuffer*) override;

  //no SharedPtr here, bz of type_cast and don't need guarantee it's lifetime in this contex
  bool DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg);
  bool EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer);
private:
  //bool EncodeHttpResponse(const HttpResponse*, IOBU* out_buffer);
  bool EncodeHttpRequest(const HttpRequest* msg, IOBuffer* out_buffer);
private:

  http_parser parser_;
  HttpParseContext parse_context_;
  static http_parser_settings parser_settings_;
};

}
#endif
