
#include "http_proto_service.h"

namespace net {

typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
typedef int (*http_cb) (http_parser*);
static int OnHttpMessageBegin(http_parser* parser) {

}

static int OnUrlParsed(http_parser* parser, const char *url_start, size_t url_len) {

}

static int OnStatusCodeParsed(http_parser* parser, const char *start, size_t len) {

}

static int OnHeaderFieldParsed(http_parser* parser, const char *header_start, size_t len) {

}

static int OnHeaderValueParsed(http_parser* parser, const char *value_start, size_t len) {

}

static int OnHeaderFinishParsed(http_parser* parser) {

}

static int OnBodyParsed(http_parser* parser, const char *body_start, size_t len) {

}

static int OnHttpMessageEnd(http_parser* parser) {

}
static int OnChunkHeader(http_parser* parser) {

}
static int OnChunkFinished(http_parser* parser) {

}

//static
http_parser_settings HttpProtoService::parser_settings_ = {
  .on_message_begin = &OnHttpMessageBegin,
  .on_url = &OnUrlParsed,
  .on_status = &OnStatusCodeParsed,
  .on_header_field = &OnHeaderFieldParsed,
  .on_header_value = &OnHeaderValueParsed,
  .on_headers_complete = &OnHeaderFinishParsed,
  .on_body = &OnBodyParsed,
  .on_message_complete = &OnHttpMessageEnd,
  .on_chunk_header = &OnChunkHeader,
  .on_chunk_complete = &OnChunkFinished,
};

HttpProtoService::HttpProtoService()
  : ProtoService("line") {
}
HttpProtoService::~HttpProtoService() {
}

void HttpProtoService::OnStatusChanged(const RefTcpChannel&) {

}
void HttpProtoService::OnDataFinishSend(const RefTcpChannel&) {
}
void HttpProtoService::OnDataRecieved(const RefTcpChannel&, IOBuffer*) {;
}

bool HttpProtoService::DecodeBufferToMessage(IOBuffer* buffer, ProtocolMessage* out_msg) {

}
bool HttpProtoService::EncodeMessageToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {

}

}//end namespace
