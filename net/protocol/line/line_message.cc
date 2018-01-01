
#include "line_message.h"

namespace net {

LineMessage::LineMessage(IODirectionType t)
  : ProtocolMessage(t, "line") {
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
