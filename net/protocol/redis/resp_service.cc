#include "io_buffer.h"
#include "resp_service.h"
#include "glog/logging.h"
#include "redis_request.h"
#include "protocol/proto_service.h"

namespace net {

RespService::RespService()
  : ProtoService("redis") {
}

RespService::~RespService() {
}

void RespService::OnStatusChanged(const RefTcpChannel& channel) {
}

void RespService::OnDataFinishSend(const RefTcpChannel&) {
}

void RespService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buffer) {
  //LOG(INFO) << " RespService Got raw data:\n" << buffer->AsString() << " next coming count:" << next_incoming_count_;
  CHECK(ProtoService::ServiceType() == kClient);

  if (!current_response) {
    current_response = std::make_shared<RedisResponse>();
  }

  do {

    resp::result res = decoder_.decode((const char*)buffer->GetRead(), buffer->CanReadSize());
    if (res != resp::completed) {
      buffer->Consume(res.size());
      break;
    }

    next_incoming_count_--;
    buffer->Consume(res.size());
    current_response->AddResult(res); 
  } while(next_incoming_count_ > 0);

  if (next_incoming_count_ == 0 && message_handler_) {
    message_handler_(std::static_pointer_cast<ProtocolMessage>(current_response));
    current_response.reset();
    next_incoming_count_ = 0; 
  }
}

bool RespService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  if (msg->ProtocolMessageType() != MessageType::kRequest) {
    LOG(ERROR) << " Only redis client side protocol supported";
    return false;
  }

  RedisRequest* request = (RedisRequest*)msg;
  out_buffer->EnsureWritableSize(request->body_.size());
  out_buffer->WriteRawData(request->body_.data(), request->body_.size());

  //LOG(INFO) << " Encode redis request to out_buffer:" << request->body_;
  return true;
}

void RespService::BeforeSendMessage(ProtocolMessage* out_message) {
  RedisRequest* req = (RedisRequest*)out_message;
  next_incoming_count_ = req->CmdCount();
}

void RespService::BeforeReplyMessage(ProtocolMessage* in, ProtocolMessage* out) {
}

} //end namespace
