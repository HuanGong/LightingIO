
#include "raw_message.h"

namespace net {
//static
const uint32_t RawMessage::kRawHeaderSize = sizeof(RawHeader);

RefRawMessage RawMessage::CreateRequest() {
  auto m = std::make_shared<RawMessage>(MessageType::kRequest);
  return std::move(m);
}
RefRawMessage RawMessage::CreateResponse() {
  auto m = std::make_shared<RawMessage>(MessageType::kResponse);
  return std::move(m);
}

RawMessage::RawMessage(MessageType t)
  : ProtocolMessage("raw", t) {
}
RawMessage::~RawMessage() {
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

void RawMessage::SetContent(const std::string& body) {
  content_ = body;
  header_.frame_size = kRawHeaderSize + content_.size();
}

void RawMessage::SetContent(const char* content) {
  content_ = content;
  header_.frame_size = kRawHeaderSize + content_.size();
}

void RawMessage::CalculateFrameSize() {
  header_.frame_size = kRawHeaderSize + content_.size();
}

const std::string RawMessage::MessageDebug() const {
  std::ostringstream oss;

  oss << "{\"type\": \"" << MessageTypeStr() << "\","
      << "\"code\": " << (int)header_.code << ","
      << "\"method\": " << (int)header_.method << ","
      << "\"frame_size\": " << (int)header_.frame_size << ","
      << "\"sequence_id\": " << (int)header_.sequence_id << ","
      << "\"content\": \"" << content_ << "\""
      << "}";

  return std::move(oss.str());
}

}//end net
