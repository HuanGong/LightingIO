
#include "line_message.h"

namespace net {

LineMessage::LineMessage()
  : ProtocolMessage("line") {
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
