
#include "glog/logging.h"
#include "parser_context.h"
#include "net/protocol/proto_message.h"

namespace net {


ResParseContext::ResParseContext()
  : last_is_header_value(false) {

  http_parser_init(&parser_, HTTP_RESPONSE);
  parser_.data = this;
}
void ResParseContext::reset() {
  current_.reset();
  half_header.first.clear();
  half_header.second.clear();
  last_is_header_value = false;
}

http_parser* ResParseContext::Parser() {
  return &parser_;
}

ReqParseContext::ReqParseContext()
  : last_is_header_value(false) {

  http_parser_init(&parser_, HTTP_REQUEST);
  parser_.data = this;
}

void ReqParseContext::reset() {
  current_.reset();
  half_header.first.clear();
  half_header.second.clear();
  last_is_header_value = false;
}

http_parser* ReqParseContext::Parser() {
  return &parser_;
}

//static
int ReqParseContext::OnHttpRequestBegin(http_parser* parser) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);

  if (context->current_) {
    LOG(ERROR) << "Something Wrong, Current Should Be Null";
  }
  context->current_ = (std::make_shared<HttpRequest>(IODirectionType::kInRequest));

  LOG(INFO) << __FUNCTION__ ;
  return 0;
}

int ReqParseContext::OnUrlParsed(http_parser* parser, const char *url_start, size_t url_len) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);

  context->current_->url_.append(url_start, url_len);

  LOG(INFO) << __FUNCTION__ << " url :" << context->current_->url_;
  return 0;
}

int ReqParseContext::OnStatusCodeParsed(http_parser* parser, const char *start, size_t len) {
  LOG(INFO) << __FUNCTION__ << " Parse Request Should Not Reached";
  return 0;
}

int ReqParseContext::OnHeaderFieldParsed(http_parser* parser, const char *header_start, size_t len) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);

  if (context->last_is_header_value) {
    if (!context->half_header.first.empty()) {
      context->current_->InsertHeader(context->half_header.first, context->half_header.second);
    }
    context->half_header.first.clear();
    context->half_header.second.clear();
  }
  context->last_is_header_value = false;
  context->half_header.first.append(header_start, len);
  return 0;
}

int ReqParseContext::OnHeaderValueParsed(http_parser* parser, const char *value_start, size_t len) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);

  context->last_is_header_value = true;
  if (context->half_header.first.empty()) {
    LOG(ERROR) << __FUNCTION__ << " Got A Empty HeaderFiled";
    return 0;
  }
  context->half_header.second.append(value_start, len);
  return 0;
}

int ReqParseContext::OnHeaderFinishParsed(http_parser* parser) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);

  if (!context->half_header.first.empty()) {
    context->current_->InsertHeader(context->half_header.first, context->half_header.second);
  }
  context->half_header.first.clear();
  context->half_header.second.clear();
  context->last_is_header_value = false;

  LOG(INFO) << __FUNCTION__ << "Finished header Parse";

  return 0;
}

int ReqParseContext::OnBodyParsed(http_parser* parser, const char *body_start, size_t len) {
  ReqParseContext* context = (ReqParseContext*)parser->data;

  context->current_->MutableBody().append(body_start, len);

  LOG(INFO) << __FUNCTION__ << "Got A Request Body";
  return 0;
}

int ReqParseContext::OnHttpRequestEnd(http_parser* parser) {
  ReqParseContext* context = (ReqParseContext*)parser->data;

  int type = parser->type;
  if (type == HTTP_REQUEST) {
    LOG(ERROR) << __FUNCTION__ << " Not A HTTP REQUEST";
  }

  context->current_->SetMessageDirection(IODirectionType::kInRequest);
  context->current_->keepalive_ = http_should_keep_alive(parser);
  context->current_->http_major_ = parser->http_major;
  context->current_->http_minor_ = parser->http_minor;

  context->current_->method_ = http_method_str((http_method)parser->method);

  //build params
  //extract host

  context->messages_.push_back(std::move(context->current_));

  context->current_.reset();

  LOG(INFO) << __FUNCTION__ ;
  return 0;
}

int ReqParseContext::OnChunkHeader(http_parser* parser) {
  return 0;
}
int ReqParseContext::OnChunkFinished(http_parser* parser) {
  return 0;
}


// response

int ResParseContext::OnHttpResponseBegin(http_parser* parser) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  if (context->current_) {
    LOG(ERROR) << "Something Wrong, Current Should Be Null";
  }
  context->current_ = (std::make_shared<HttpResponse>(IODirectionType::kInReponse));

  LOG(INFO) << __FUNCTION__ << " A HttpResponse Begin";
  return 0;
}
int ResParseContext::OnUrlParsed(http_parser* parser, const char *url_start, size_t url_len) {
  LOG(INFO) << __FUNCTION__ << " Parse Request Should Not Reached";
  return 0;
}
int ResParseContext::OnStatusCodeParsed(http_parser* parser, const char *start, size_t len) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  LOG(INFO) << __FUNCTION__ << " status code:" << std::string(start, len);
  //context->current_->status_code_ =
  return 0;
}

int ResParseContext::OnHeaderFieldParsed(http_parser* parser, const char *header_start, size_t len) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  if (context->last_is_header_value) {
    if (!context->half_header.first.empty()) {
      context->current_->InsertHeader(context->half_header.first, context->half_header.second);
    }
    context->half_header.first.clear();
    context->half_header.second.clear();
  }
  context->last_is_header_value = false;
  context->half_header.first.append(header_start, len);

  return 0;
}

int ResParseContext::OnHeaderValueParsed(http_parser* parser, const char *value_start, size_t len) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  context->last_is_header_value = true;
  if (context->half_header.first.empty()) {
    LOG(ERROR) << __FUNCTION__ << " Got A Empty HeaderFiled";
    return 0;
  }
  context->half_header.second.append(value_start, len);
  return 0;
}

int ResParseContext::OnHeaderFinishParsed(http_parser* parser) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  if (!context->half_header.first.empty()) {
    context->current_->InsertHeader(context->half_header.first, context->half_header.second);
  }
  context->half_header.first.clear();
  context->half_header.second.clear();
  context->last_is_header_value = false;

  LOG(INFO) << __FUNCTION__ << "Finished header Parse";

  return 0;
}

int ResParseContext::OnBodyParsed(http_parser* parser, const char *body_start, size_t len) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  context->current_->body_.append(body_start, len);
  LOG(INFO) << __FUNCTION__ << "Got A Response Body";
  return 0;
}
int ResParseContext::OnHttpResponseEnd(http_parser* parser) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  int type = parser->type;
  if (type == HTTP_RESPONSE) {
    LOG(ERROR) << __FUNCTION__ << " Not A HTTP_RESPONSE";
  }

  context->current_->SetMessageDirection(IODirectionType::kInReponse);
  context->current_->keepalive_ = http_should_keep_alive(parser);
  context->current_->http_major_ = parser->http_major;
  context->current_->http_minor_ = parser->http_minor;
  context->current_->status_code_ = parser->status_code;

  context->messages_.push_back(std::move(context->current_));
  context->current_.reset();

  LOG(INFO) << __FUNCTION__ << " Get A Full HTTP_RESPONSE Message";
  return 0;
}

int ResParseContext::OnChunkHeader(http_parser* parser) {
  LOG(INFO) << __FUNCTION__ ;
  return 0;
}
int ResParseContext::OnChunkFinished(http_parser* parser) {
  LOG(INFO) << __FUNCTION__ ;
  return 0;
}

}
