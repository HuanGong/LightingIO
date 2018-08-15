
#include "proto_message.h"

namespace net {

ProtocolMessage::ProtocolMessage(const std::string protocol)
  : fail_info_(kNothing),
    proto_(protocol),
    type_(kNone) {
  work_context_.loop = NULL;
  work_context_.task_identify = 0;
}

ProtocolMessage::~ProtocolMessage() {
}

const std::string& ProtocolMessage::Protocol() const {
  return proto_;
}

void ProtocolMessage::SetFailInfo(FailInfo reason) {
  fail_info_ = reason;
}
FailInfo ProtocolMessage::MessageFailInfo() const {
  return fail_info_;
}

void ProtocolMessage::SetIOContextWeakChannel(const RefTcpChannel& channel) {
  io_context_.channel = channel;
}
void ProtocolMessage::SetIOContextWeakChannel(WeakPtrTcpChannel& channel) {
  io_context_.channel = channel;
}

void ProtocolMessage::SetResponse(RefProtocolMessage&& response) {
  response_ = response;
}

void ProtocolMessage::SetResponse(RefProtocolMessage& response) {
  response_ = response;
}

const std::string& ProtocolMessage::FailMessage() const {
  return fail_;
}

void ProtocolMessage::AppendFailMessage(std::string m) {
  if (!fail_.empty()) {
    fail_.append("->").append(m);
    return;
  }
  fail_ = m;
}

const char* ProtocolMessage::MessageTypeStr() const {
  switch(type_) {
    case MessageType::kRequest:
      return "Request Message";
    case MessageType::kResponse:
      return "Response Message";
    case MessageType::kNone:
      return "MessageNone";
    default:
      return "MessageNone";
  }
  return "MessageNone";
}

}//end namespace
