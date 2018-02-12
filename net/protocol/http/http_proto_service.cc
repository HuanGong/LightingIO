
#include "glog/logging.h"

#include "tcp_channel.h"
#include "net/io_buffer.h"
#include "http_proto_service.h"
#include "http_constants.h"
#include "net/protocol/proto_message.h"
#include "base/compression_utils/gzip_utils.h"

namespace net {

static const int32_t kMeanHeaderSize = 24;
static const int32_t kHttpMsgReserveSize = 64;

//static
http_parser_settings HttpProtoService::req_parser_settings_ = {
  .on_message_begin = &ReqParseContext::OnHttpRequestBegin,
  .on_url = &ReqParseContext::OnUrlParsed,
  .on_status = &ReqParseContext::OnStatusCodeParsed,
  .on_header_field = &ReqParseContext::OnHeaderFieldParsed,
  .on_header_value = &ReqParseContext::OnHeaderValueParsed,
  .on_headers_complete = &ReqParseContext::OnHeaderFinishParsed,
  .on_body = &ReqParseContext::OnBodyParsed,
  .on_message_complete = &ReqParseContext::OnHttpRequestEnd,
  .on_chunk_header = &ReqParseContext::OnChunkHeader,
  .on_chunk_complete = &ReqParseContext::OnChunkFinished,
};

http_parser_settings HttpProtoService::res_parser_settings_ = {
  .on_message_begin = &ResParseContext::OnHttpResponseBegin,
  .on_url = &ResParseContext::OnUrlParsed,
  .on_status = &ResParseContext::OnStatusCodeParsed,
  .on_header_field = &ResParseContext::OnHeaderFieldParsed,
  .on_header_value = &ResParseContext::OnHeaderValueParsed,
  .on_headers_complete = &ResParseContext::OnHeaderFinishParsed,
  .on_body = &ResParseContext::OnBodyParsed,
  .on_message_complete = &ResParseContext::OnHttpResponseEnd,
  .on_chunk_header = &ResParseContext::OnChunkHeader,
  .on_chunk_complete = &ResParseContext::OnChunkFinished,
};

HttpProtoService::HttpProtoService()
  : ProtoService("http"),
    request_context_(NULL),
    response_context_(NULL) {

  request_context_ = new ReqParseContext();
  response_context_ = new ResParseContext();
}

HttpProtoService::~HttpProtoService() {
  delete request_context_;
  delete response_context_;
}

void HttpProtoService::OnStatusChanged(const RefTcpChannel& channel) {
}

void HttpProtoService::OnDataFinishSend(const RefTcpChannel& channel) {
}

void HttpProtoService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buf) {;
  bool success = false;
  if (channel->IsServerChannel()) {// parse Request
    success = ParseHttpRequest(channel, buf);
  } else {
    success = ParseHttpResponse(channel, buf);
  }

  if (!success) {
    channel->ShutdownChannel();
  }
}

bool HttpProtoService::DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg) {
  return false;
}

bool HttpProtoService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  CHECK(msg && out_buffer);
  switch(msg->MessageDirection()) {
    case IODirectionType::kInRequest:
    case IODirectionType::kOutRequest: {
      const HttpRequest* request = static_cast<const HttpRequest*>(msg);
      return RequestToBuffer(request, out_buffer);
    } break;
    case IODirectionType::kInReponse:
    case IODirectionType::kOutResponse: {
      const HttpResponse* response = static_cast<const HttpResponse*>(msg);
      return ResponseToBuffer(response, out_buffer);
    } break;
    default: {
      LOG(ERROR) << "Should Not Readched";
    } break;
  }
  return false;
}

bool HttpProtoService::ParseHttpRequest(const RefTcpChannel& channel, IOBuffer* buf) {
  size_t buffer_size = buf->CanReadSize();
  const char* buffer_start = (const char*)buf->GetRead();
  http_parser* parser = request_context_->Parser();


  size_t nparsed = http_parser_execute(parser,
                                       &req_parser_settings_,
                                       buffer_start,
                                       buffer_size);

  buf->Consume(nparsed);

  if (parser->upgrade) {
    LOG(ERROR) << " Not Supported Now";

    request_context_->current_.reset();
    channel->Send((const uint8_t*)HttpConstant::kBadRequest.data(), HttpConstant::kBadRequest.size());

    return false;
  } else if (nparsed != buffer_size) {
    LOG(ERROR) << " Parse Occur ERROR, nparsed" << nparsed
               << " buffer_size:" << buffer_size;

    request_context_->current_.reset();
    channel->Send((const uint8_t*)HttpConstant::kBadRequest.data(), HttpConstant::kBadRequest.size());

    return false;
  }

  while (request_context_->messages_.size()) {
    RefHttpRequest message = request_context_->messages_.front();
    request_context_->messages_.pop_front();

    message->SetIOContextWeakChannel(channel);
    message->SetMessageDirection(IODirectionType::kInRequest);

    if (message_handler_) {
      message_handler_(std::static_pointer_cast<ProtocolMessage>(message));
    } else {
      channel->ShutdownChannel();
    }
  }
  return true;
}

bool HttpProtoService::ParseHttpResponse(const RefTcpChannel& channel, IOBuffer* buf) {

  size_t buffer_size = buf->CanReadSize();
  const char* buffer_start = (const char*)buf->GetRead();
  http_parser* parser = response_context_->Parser();
  size_t nparsed = http_parser_execute(parser,
                                       &res_parser_settings_,
                                       buffer_start,
                                       buffer_size);

  if (parser->upgrade) { //websockt
    LOG(ERROR) << " Not Supported Now";
    response_context_->current_.reset();
    return false;
  } else if (nparsed != buffer_size) {
    LOG(ERROR) << " Parse Occur ERROR";
    response_context_->current_.reset();
    return false;
  }
  buf->Consume(nparsed);

  while (response_context_->messages_.size()) {
    RefHttpResponse message = response_context_->messages_.front();
    response_context_->messages_.pop_front();

    message->SetIOContextWeakChannel(channel);
    message->SetMessageDirection(IODirectionType::kInRequest);

    if (message_handler_) {
      message_handler_(std::static_pointer_cast<ProtocolMessage>(message));
    }
  }
  return true;
}

//static
bool HttpProtoService::RequestToBuffer(const HttpRequest* request, IOBuffer* buffer) {
  CHECK(request && buffer);
  int32_t guess_size = kHttpMsgReserveSize + request->Body().size();
  guess_size += request->Headers().size() * kMeanHeaderSize;
  buffer->EnsureWritableSize(guess_size);

  buffer->WriteString(request->Method());
  buffer->WriteString(HttpConstant::kBlankSpace);
  buffer->WriteString(request->RequestUrl());
  buffer->WriteString(HttpConstant::kBlankSpace);

  char v[11]; //"HTTP/1.1\r\n"
  snprintf(v, 11, "HTTP/%d.%d\r\n", request->VersionMajor(), request->VersionMinor());
  buffer->WriteRawData(v, 10);

  for (const auto& header : request->Headers()) {
    buffer->WriteString(header.first);
    buffer->WriteRawData(": ", 2);
    buffer->WriteString(header.second);
    buffer->WriteString(HttpConstant::kCRCN);
  }

  if (!request->HasHeaderField(HttpConstant::kConnection)) {
    buffer->WriteString(request->IsKeepAlive() ?
                        HttpConstant::kHeaderKeepAlive :
                        HttpConstant::kHeaderNotKeepAlive);
  }

  if (!request->HasHeaderField(HttpConstant::kAcceptEncoding)) {
    buffer->WriteString(HttpConstant::kHeaderSupportedEncoding);
  }

  if (!request->HasHeaderField(HttpConstant::kContentLength)) {
    buffer->WriteString(HttpConstant::kContentLength);
    std::string content_len(": ");
    content_len.append(std::to_string(request->Body().size()));
    content_len.append(HttpConstant::kCRCN);
    buffer->WriteString(content_len);
  }

  if (!request->HasHeaderField(HttpConstant::kContentType)) {
    buffer->WriteString(HttpConstant::kHeaderDefaultContentType);
  }
  buffer->WriteString(HttpConstant::kCRCN);

  buffer->WriteString(request->Body());
  return true;
}

//static
bool HttpProtoService::ResponseToBuffer(const HttpResponse* response, IOBuffer* buffer) {
  CHECK(response && buffer);

  int32_t guess_size = kHttpMsgReserveSize + response->Headers().size() * kMeanHeaderSize + response->Body().size();
  buffer->EnsureWritableSize(guess_size);

  //char v[14]; //"HTTP/1.1 xxx "
  //snprintf(v, 13, "HTTP/%d.%d %d ", (int)response->VersionMajor(), (int)response->VersionMinor(), response->ResponseCode());
  std::string headline("HTTP/1.1 ");
  headline.append(std::to_string(response->ResponseCode()));
  headline.append(" ");
  headline.append(HttpConstant::StatusCodeCStr(response->ResponseCode()));
  headline.append(HttpConstant::kCRCN);
  buffer->WriteString(headline);

  for (const auto& header : response->Headers()) {
    buffer->WriteString(header.first);
    buffer->WriteRawData(": ", 2);
    buffer->WriteString(header.second);
    buffer->WriteString(HttpConstant::kCRCN);
  }

  if (!response->HasHeaderField(HttpConstant::kConnection)) {
    buffer->WriteString(response->IsKeepAlive() ?
                        HttpConstant::kHeaderKeepAlive :
                        HttpConstant::kHeaderNotKeepAlive);
  }

  if (!response->HasHeaderField(HttpConstant::kContentLength)) {
    std::string content_len(HttpConstant::kContentLength);
    content_len.append(": ");
    content_len.append(std::to_string(response->Body().size()));
    content_len.append(HttpConstant::kCRCN);
    buffer->WriteString(content_len);
  }

  if (!response->HasHeaderField(HttpConstant::kContentType)) {
    buffer->WriteString(HttpConstant::kHeaderDefaultContentType);
  }
  buffer->WriteString(HttpConstant::kCRCN);

  buffer->WriteString(response->Body());
  return true;
}

const RefProtocolMessage HttpProtoService::DefaultResponse(const RefProtocolMessage& request)  {
  CHECK(request);
  HttpRequest* http_request = static_cast<HttpRequest*>(request.get());

  const RefHttpResponse http_res =
    std::make_shared<HttpResponse>(IODirectionType::kOutResponse);

  http_res->SetResponseCode(500);
  http_res->SetKeepAlive(http_request->IsKeepAlive());

  return std::move(http_res);
}

bool HttpProtoService::CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) {
  if (!request) {
    return true;
  }
  HttpRequest* http_request = static_cast<HttpRequest*>(request);
  //HttpResponse* http_response = static_cast<HttpResponse*>(response);
  if (http_request->IsKeepAlive()) {
    return false;
  }
  return true;
}

void HttpProtoService::BeforeSendMessage(ProtocolMessage* out_message) {
  if (out_message->MessageDirection() != IODirectionType::kOutRequest) {
    return;
  }
  HttpRequest* request = static_cast<HttpRequest*>(out_message);
  if (request->Body().size() < 4096 ||
      request->HasHeaderField(HttpConstant::kContentEncoding)) {
    return;
  }
  //compress the body default
  std::string compressed_body;
  if (0 == base::Gzip::compress_gzip(request->Body(), compressed_body)) { //success
    request->body_ = std::move(compressed_body);
    request->InsertHeader("Content-Encoding", "gzip");
  }
}

void HttpProtoService::BeforeReplyMessage(ProtocolMessage* in, ProtocolMessage* out) {
  HttpRequest* request = static_cast<HttpRequest*>(in);
  HttpResponse* response = static_cast<HttpResponse*>(out);

  //response compression if needed
  if (response->Body().size() > 4096 &&
      !response->HasHeaderField(HttpConstant::kContentEncoding)) {

    const std::string& accept = request->GetHeader(HttpConstant::kAcceptEncoding);
    std::string compressed_body;
    if (accept.find("gzip") != std::string::npos) {
      if (0 == base::Gzip::compress_gzip(response->Body(), compressed_body)) { //success
        response->body_ = std::move(compressed_body);
        response->InsertHeader("Content-Encoding", "gzip");
      }
    } else if (accept.find("deflate") != std::string::npos) {
      if (0 == base::Gzip::compress_deflate(response->Body(), compressed_body)) {
        response->body_ = std::move(compressed_body);
        response->InsertHeader("Content-Encoding", "deflate");
      }
    }
  }

}

}//end namespace
