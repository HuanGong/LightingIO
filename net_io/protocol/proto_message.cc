
#include "proto_message.h"

namespace net {

const RefProtocolMessage ProtocolMessage::kNullMessage = RefProtocolMessage();

ProtocolMessage::ProtocolMessage(const std::string protocol, MessageType type)
  : type_(type),
    proto_(protocol),
    fail_code_(kSuccess),
    responded_(false) {
  work_context_.loop = NULL;
}

ProtocolMessage::~ProtocolMessage() {
}

const std::string& ProtocolMessage::Protocol() const {
  return proto_;
}

void ProtocolMessage::SetFailCode(MessageCode reason) {
  fail_code_ = reason;
}
MessageCode ProtocolMessage::FailCode() const {
  return fail_code_;
}

void ProtocolMessage::SetIOContext(const RefProtoService& service) {
  io_context_.protocol_service = service;
}

void ProtocolMessage::SetResponse(const RefProtocolMessage& response) {
  response_ = response;
  responded_ = true;
}

const char* ProtocolMessage::TypeAsStr() const {
  switch(type_) {
    case MessageType::kRequest:
      return "Request";
    case MessageType::kResponse:
      return "Response";
    default:
      break;
  }
  return "MessageNone";
}

}//end namespace
