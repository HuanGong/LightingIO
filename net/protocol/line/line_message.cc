
#include "line_message.h"

namespace net {

LineMessage::LineMessage(ProtoMsgType t)
  : ProtocolMessage("line") {
  SetMessageType(t);
}

LineMessage::~LineMessage() {

}
std::string& LineMessage::MutableBody() {
  return body_;
}
const std::string& LineMessage::Body() const {
  return body_;
}

};//end namespace net
