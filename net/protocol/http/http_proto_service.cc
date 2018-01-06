
#include "glog/logging.h"

#include "tcp_channel.h"
#include "net/io_buffer.h"
#include "http_proto_service.h"
#include "net/protocol/proto_message.h"

namespace net {

//static
int HttpProtoService::OnHttpRequestBegin(http_parser* parser) {
  HttpParseContext* context = (HttpParseContext*)parser->data;
  CHECK(context);

  context->current_ = (std::make_shared<HttpRequest>(IODirectionType::kUnknownType));
  LOG(INFO) << __FUNCTION__ ;
  return 0;
}

//static
int HttpProtoService::OnUrlParsed(http_parser* parser, const char *url_start, size_t url_len) {
  HttpParseContext* context = (HttpParseContext*)parser->data;
  CHECK(context);

  context->current_->url_.append(url_start, url_len);

  LOG(INFO) << __FUNCTION__ << " url :" << context->current_->url_;
  return 0;
}

//static
int HttpProtoService::OnStatusCodeParsed(http_parser* parser, const char *start, size_t len) {
  HttpParseContext* context = (HttpParseContext*)parser->data;
  CHECK(context);

  context->status_code.append(start, len);
  LOG(INFO) << __FUNCTION__ << " status code:" << context->status_code;
  return 0;
}

//static
int HttpProtoService::OnHeaderFieldParsed(http_parser* parser, const char *header_start, size_t len) {
  HttpParseContext* context = (HttpParseContext*)parser->data;

  std::string header_field(header_start, len);
  LOG(INFO) << __FUNCTION__ << " header field:" << header_field;
  if (context->last_is_header_value) {
    if (!context->half_header.first.empty()) {
      context->current_->InsertHeader(context->half_header.first, context->half_header.second);
    }
    context->half_header.first.clear();
    context->half_header.second.clear();
    context->last_is_header_value = false;
  }

  context->half_header.first.append(header_start, len);
  return 0;
}

//static
int HttpProtoService::OnHeaderValueParsed(http_parser* parser, const char *value_start, size_t len) {
  HttpParseContext* context = (HttpParseContext*)parser->data;

  context->last_is_header_value = true;
  if (context->half_header.first.empty()) {
    LOG(ERROR) << __FUNCTION__ << " should Not Reach Here:";
    return 0;
  }
  context->half_header.second.append(value_start, len);

  std::string header_value(value_start, len);

  LOG(INFO) << __FUNCTION__ << " header value:" << header_value;
  return 0;
}

//static
int HttpProtoService::OnHeaderFinishParsed(http_parser* parser) {

  HttpParseContext* context = (HttpParseContext*)parser->data;

  if (!context->half_header.first.empty()) {
    context->current_->InsertHeader(context->half_header.first, context->half_header.second);
  }
  context->half_header.first.clear();
  context->half_header.second.clear();
  context->last_is_header_value = false;

  LOG(INFO) << __FUNCTION__ << " header finish ";

  return 0;
}

//static
int HttpProtoService::OnBodyParsed(http_parser* parser, const char *body_start, size_t len) {
  HttpParseContext* context = (HttpParseContext*)parser->data;

  context->current_->MutableBody().append(body_start, len);

  std::string body_value(body_start, len);
  LOG(INFO) << __FUNCTION__ << " body value:" << body_value;
  return 0;
}

//static
int HttpProtoService::OnHttpRequestEnd(http_parser* parser) {

  HttpParseContext* context = (HttpParseContext*)parser->data;

  int type = parser->type;
  IODirectionType direction =
    (type == HTTP_REQUEST) ? IODirectionType::kInRequest : IODirectionType::kInReponse;
  context->current_->SetMessageDirection(direction);
  context->current_->keepalive_ = http_should_keep_alive(parser);
  context->current_->http_major_ = parser->http_major;
  context->current_->http_minor_ = parser->http_minor;
  context->current_->method_ = http_method_str((http_method)parser->method);

  //build params
  //extract host

  context->messages_.push_back(std::move(context->current_));
  LOG(INFO) << __FUNCTION__ ;
  return 0;
}
//static
int HttpProtoService::OnChunkHeader(http_parser* parser) {
  LOG(INFO) << __FUNCTION__ ;
  return 0;
}
//static
int HttpProtoService::OnChunkFinished(http_parser* parser) {
  LOG(INFO) << __FUNCTION__ ;
  return 0;
}

//static
http_parser_settings HttpProtoService::parser_settings_ = {
  .on_message_begin = &HttpProtoService::OnHttpRequestBegin,
  .on_url = &HttpProtoService::OnUrlParsed,
  .on_status = &HttpProtoService::OnStatusCodeParsed,
  .on_header_field = &HttpProtoService::OnHeaderFieldParsed,
  .on_header_value = &HttpProtoService::OnHeaderValueParsed,
  .on_headers_complete = &HttpProtoService::OnHeaderFinishParsed,
  .on_body = &HttpProtoService::OnBodyParsed,
  .on_message_complete = &HttpProtoService::OnHttpRequestEnd,
  .on_chunk_header = &HttpProtoService::OnChunkHeader,
  .on_chunk_complete = &HttpProtoService::OnChunkFinished,
};

HttpProtoService::HttpProtoService()
  : ProtoService("line") {

  http_parser_init(&parser_, HTTP_BOTH);
  parser_.data = &parse_context_;
}

HttpProtoService::~HttpProtoService() {
}

void HttpProtoService::OnStatusChanged(const RefTcpChannel&) {
  LOG(INFO) << __FUNCTION__ ;
}

void HttpProtoService::OnDataFinishSend(const RefTcpChannel&) {
  LOG(INFO) << __FUNCTION__ ;
}

void HttpProtoService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buf) {;

  size_t buffer_size = buf->CanReadSize();
  const char* buffer_start = (const char*)buf->GetRead();
  size_t nparsed = http_parser_execute(&parser_,
                                       &parser_settings_,
                                       buffer_start,
                                       buffer_size);
  LOG(ERROR) << "recv:" << buf->AsString() << "nparsed:" << nparsed << " buffer_size:" << buffer_size;

  if (parser_.upgrade) {
    LOG(ERROR) << " Not Supported Now";
    if (parse_context_.current_) {
      parse_context_.current_.reset();
      channel->ShutdownChannel();
    }
  } else if (nparsed != buffer_size) {
    LOG(ERROR) << " Parse Occur ERROR";
    parse_context_.current_.reset();
    channel->ShutdownChannel();
  }
  buf->Consume(nparsed);

  for (uint32_t i = 0; i < parse_context_.messages_.size(); i++) {

    RefHttpRequest& message = parse_context_.messages_[i];

    message->SetIOContextWeakChannel(channel);

    IODirectionType direction =
      channel->IsServicerChannel() ? IODirectionType::kInRequest : IODirectionType::kInReponse;
    message->SetMessageDirection(direction);

    InvokeMessageHandler(message);
  }
  parse_context_.messages_.clear();
}

bool HttpProtoService::DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg) {
return false;
}
bool HttpProtoService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  CHECK(msg && out_buffer);
  return false;
}

bool HttpProtoService::EncodeHttpRequest(const HttpRequest* msg, IOBuffer* out_buffer) {

}

}//end namespace
