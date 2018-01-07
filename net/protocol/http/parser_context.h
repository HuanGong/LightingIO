
#ifndef _NET_HTTP_PROTO_PARSER_CONTEXT_H_
#define _NET_HTTP_PROTO_PARSER_CONTEXT_H_

#include <string>
#include <list>
#include <vector>

#include "http_request.h"
#include "http_response.h"
#include "http_parser/http_parser.h"

namespace net {

//response for build HttpResponse
class ResParseContext {
public:
  ResParseContext();

  void reset();
  http_parser* Parser();

  static int OnHttpResponseBegin(http_parser* parser);
  static int OnUrlParsed(http_parser* parser, const char *url_start, size_t url_len);
  static int OnStatusCodeParsed(http_parser* parser, const char *start, size_t len);
  static int OnHeaderFieldParsed(http_parser* parser, const char *header_start, size_t len);
  static int OnHeaderValueParsed(http_parser* parser, const char *value_start, size_t len);
  static int OnHeaderFinishParsed(http_parser* parser);
  static int OnBodyParsed(http_parser* parser, const char *body_start, size_t len);
  static int OnHttpResponseEnd(http_parser* parser);
  static int OnChunkHeader(http_parser* parser);
  static int OnChunkFinished(http_parser* parser);

private:
  friend class HttpProtoService;
  http_parser parser_;

  bool last_is_header_value;
  std::pair<std::string, std::string> half_header;
  RefHttpResponse current_;
  std::list<RefHttpResponse> messages_;
};

class ReqParseContext {
public:
  ReqParseContext();

  void reset();
  http_parser* Parser();

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

private:
  friend class HttpProtoService;
  http_parser parser_;

  bool last_is_header_value;
  std::pair<std::string, std::string> half_header;
  RefHttpRequest current_;
  std::list<RefHttpRequest> messages_;
};

}
#endif
