
#include "proto_message.h"

namespace net {

ProtocolMessage::ProtocolMessage(IODirectionType direction, const std::string protocol)
  : fail_info_(kNothing),
    proto_(protocol),
    direction_(direction) {
  work_context_.coro_loop = NULL;
}

ProtocolMessage::~ProtocolMessage() {
}

void ProtocolMessage::SetMessageDirection(IODirectionType t) {
  direction_ = t;
}

const IODirectionType ProtocolMessage::MessageDirection() const {
  return direction_;
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
  if (direction_ == IODirectionType::kInReponse ||
      direction_ == IODirectionType::kOutResponse) {
    return;
  }
  response_ = response;
}

void ProtocolMessage::SetResponse(RefProtocolMessage& response) {
  if (direction_ == IODirectionType::kInReponse ||
      direction_ == IODirectionType::kOutResponse) {
    return;
  }
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

const char* ProtocolMessage::DirectionTypeStr() const {
  switch(MessageDirection()) {
    case IODirectionType::kInRequest:
      return "HttpRequest In";
    case IODirectionType::kOutRequest:
      return "HttpRequest OUT";
    case IODirectionType::kOutResponse:
      return "HttpResponse OUT";
    case IODirectionType::kInReponse:
      return "HttpResponse IN";
    default:
      return "kUnknownType";
  }
  return "kUnknownType";
}

}//end namespace
