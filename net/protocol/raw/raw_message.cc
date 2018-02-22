
#include "raw_message.h"

namespace net {

RawMessage::RawMessage(IODirectionType t)
  : ProtocolMessage(t ,"raw") {
}
RawMessage::~RawMessage() {
}

//std::string& RawMessage::MutableContent() {
  //return content_;
//}

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

void RawMessage::SetContent(const std::string& body) {
  static const uint32_t header_size = sizeof(RawHeader);

  content_ = body;
  header_.frame_size = header_size + body.size();
}

void RawMessage::SetSequenceId(uint32_t sequence_id) {
  header_.sequence_id = sequence_id;
}

const std::string RawMessage::MessageDebug() const {
  std::ostringstream oss;

  oss << "{\"type\": \"" << DirectionTypeStr() << "\","
      << "\"code\": " << (int)header_.code << ","
      << "\"method\": " << (int)header_.method << ","
      << "\"frame_size\": " << (int)header_.frame_size << ","
      << "\"sequence_id\": " << (int)header_.sequence_id << ","
      << "\"content\": \"" << content_ << "\""
      << "}";

  return std::move(oss.str());
}

}//end net
