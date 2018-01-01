
#include "proto_message.h"

namespace net {

ProtocolMessage::ProtocolMessage(const std::string protocol)
  : proto_(protocol) {
}

ProtocolMessage::~ProtocolMessage() {
}

void ProtocolMessage::SetMessageType(ProtoMsgType t) {
  type_ = t;
}

const ProtoMsgType ProtocolMessage::MessageType() const {
  return type_;
}

const std::string& ProtocolMessage::Protocol() const {
  return proto_;
}

void ProtocolMessage::SetIOContextWeakChannel(const RefTcpChannel& channel) {
  io_context_.channel = channel;
}
void ProtocolMessage::SetIOContextWeakChannel(WeakPtrTcpChannel& channel) {
  io_context_.channel = channel;
}

void ProtocolMessage::SetResponse(RefProtocolMessage&& response) {
  if (type_ == ProtoMsgType::kInReponse ||
      type_ == ProtoMsgType::kOutResponse) {
    return;
  }
  response_ = response;
}

void ProtocolMessage::SetResponse(RefProtocolMessage& response) {
  if (type_ == ProtoMsgType::kInReponse ||
      type_ == ProtoMsgType::kOutResponse) {
    return;
  }
  response_ = response;
}

}//end namespace
