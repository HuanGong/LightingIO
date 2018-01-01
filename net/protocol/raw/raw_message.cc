
#include "raw_message.h"

namesapce net {

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

void RawMessage::SetMethod(uint8_t method) {
  header_.method = method;
}
uint8_t RawMessage::Method() const {
  return header_.method;
}
void RawMessage::SetSequenceId(uint8_t seq_id) {
  header_.sequence_id= seq_id;
}
uint32_t RawMessage::SequenceId() const {
  return header_.sequence_id;
}
void RawMessage::SetFrameSize(uint32_t size) {
  header_.frame_size = size;
}
uint32_t RawMessage::FrameSize() const {
  return header_.frame_size;
}

}//end net
