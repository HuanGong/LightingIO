
#include "glog/logging.h"

#include "tcp_channel.h"
#include "net/io_buffer.h"
#include "http_proto_service.h"
#include "http_constants.h"
#include "net/protocol/proto_message.h"

namespace net {

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
    response_context_(NULL),
    close_after_finish_send_(false) {

  request_context_ = new ReqParseContext();
  response_context_ = new ResParseContext();
}

HttpProtoService::~HttpProtoService() {
  delete request_context_;
  delete response_context_;
}

void HttpProtoService::OnStatusChanged(const RefTcpChannel& channel) {
  LOG(INFO) << __FUNCTION__ << channel->ChannelName() << " status:" << channel->StatusAsString();
}

void HttpProtoService::OnDataFinishSend(const RefTcpChannel& channel) {
  LOG(INFO) << __FUNCTION__ << channel->ChannelName();
  if (close_after_finish_send_) {
    channel->ShutdownChannel();
  }
}

void HttpProtoService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buf) {;
  //static const std::string kBadRequest = "HTTP/1.1 400 Bad Request\r\n\r\n";

  LOG(ERROR) << " OnDataRecieved n bytes:" << buf->CanReadSize() << " From:" << channel->ChannelName();

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
    //TODO: consider add a uniform interface for httpmessge, just a ToHttpRawData api ?
    case IODirectionType::kInRequest:
    case IODirectionType::kOutRequest: {
      std::ostringstream oss;
      const HttpRequest* request = static_cast<const HttpRequest*>(msg);
      request->ToRequestRawData(oss);
      out_buffer->WriteString(oss.str());
      return true;
    } break;
    case IODirectionType::kInReponse:
    case IODirectionType::kOutResponse: {
      std::ostringstream oss;
      const HttpResponse* request = static_cast<const HttpResponse*>(msg);
      request->ToResponseRawData(oss);
      out_buffer->WriteString(oss.str());
      return true;
    } break;
    default:
      return false;
    break;
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

  LOG(ERROR) << " reqeuest as string:" << buf->AsString();
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

    //Config IOContext
    message->SetIOContextWeakChannel(channel);
    message->SetMessageDirection(IODirectionType::kInRequest);

    PlayRequest(message);
#if 0
    if (dispatcher_) {
      LOG(ERROR) << "has dispatcher_";
      RefHttpProtoService service = std::static_pointer_cast<HttpProtoService>(channel->GetProtoService());
      base::StlClourse clourse = std::bind(&HttpProtoService::PlayRequest, service, message);
      dispatcher_->Play(clourse);
    } else { //play itself
      LOG(ERROR) << "no dispatcher_";
      PlayRequest(message);
    }
#endif
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

  if (parser->upgrade) {
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

    LOG(ERROR) << " PlayResponse ";
    PlayResponse(message);
    //InvokeMessageHandler(message);
  }
  return true;
}

void HttpProtoService::PlayRequest(RefHttpRequest request) {

  if (message_handler_) {
    message_handler_(std::static_pointer_cast<ProtocolMessage>(request));
  }

  WeakPtrTcpChannel weak_channel = request->GetIOCtx().channel;
  RefTcpChannel channel = weak_channel.lock();
  if (!channel.get() && channel->IsConnected()) {
    return;
  }

  RefProtocolMessage response = request->Response();
  if (!response.get()) {
    RefHttpResponse http_res = std::make_shared<HttpResponse>(IODirectionType::kOutResponse);
    http_res->MutableBody() = HttpConstant::kBadRequest;
    if (!request->IsKeepAlive()) {
      http_res->SetKeepAlive(request->IsKeepAlive());
    }
    response = std::static_pointer_cast<ProtocolMessage>(http_res);
  }
  HttpResponse* http_res = static_cast<HttpResponse*>(response.get());
  if (!http_res->IsKeepAlive()) {
    close_after_finish_send_ = true;
  } else {
    close_after_finish_send_ = false;
  }

  if (channel->InIOLoop()) {
    channel->SendProtoMessage(response);
  } else {
    auto functor = std::bind(&TcpChannel::SendProtoMessage, channel, response);
    channel->IOLoop()->PostTask(base::NewClosure(std::move(functor)));
  }
}

void HttpProtoService::PlayResponse(RefHttpResponse response) {
  LOG(ERROR) << __FUNCTION__ << " Handle A Response";
  if (message_handler_) {
    message_handler_(std::static_pointer_cast<ProtocolMessage>(response));
  }
}

}//end namespace
