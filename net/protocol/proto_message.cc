
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

}//end namespace
