
#include "raw_message.h"

namespace net {

RawMessage::RawMessage(IODirectionType t)
  : ProtocolMessage(t ,"raw") {
}
RawMessage::~RawMessage() {
}

std::string& RawMessage::MutableContent() {
  return content_;
}
const std::string& RawMessage::Content() const {
  return content_;
}

void RawMessage::SetCode(uint8_t code) {
  header_.code = code;
}

void RawMessage::SetMethod(uint8_t method) {
  header_.method = method;
}

void RawMessage::SetFrameSize(uint32_t frame_size) {
  header_.frame_size = frame_size;
}

void RawMessage::SetSequenceId(uint32_t sequence_id) {
  header_.sequence_id = sequence_id;
}

}//end net
