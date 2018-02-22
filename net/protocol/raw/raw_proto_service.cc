
#include "io_buffer.h"
#include "tcp_channel.h"
#include "glog/logging.h"
#include "raw_message.h"
#include "raw_proto_service.h"

namespace net {

static const uint32_t kRawHeaderSize = sizeof(RawHeader);
RawProtoService::RawProtoService()
  : ProtoService("raw") {
}

RawProtoService::~RawProtoService() {

}

  // override from ProtoService
void RawProtoService::OnStatusChanged(const RefTcpChannel&) {
}

void RawProtoService::OnDataFinishSend(const RefTcpChannel&) {
}

void RawProtoService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buffer) {
  do {
    if (buffer->CanReadSize() <= (int)kRawHeaderSize) {
      return;
    }
    const RawHeader* header = (const RawHeader*)buffer->GetRead();
    int32_t frame_size = header->frame_size;
    int32_t body_size = frame_size - kRawHeaderSize;

    if (buffer->CanReadSize() < frame_size) {
      return;
    }

    IODirectionType type = channel->IsServerChannel() ?
      IODirectionType::kInRequest : IODirectionType::kInReponse;

    auto raw_message = std::make_shared<RawMessage>(type);
    raw_message->SetIOContextWeakChannel(channel);
    memcpy(&(raw_message->header_), buffer->GetRead(), kRawHeaderSize);
    buffer->Consume(kRawHeaderSize);

    std::string& content = raw_message->MutableContent();
    content.append((const char*)buffer->GetRead(), body_size);
    buffer->Consume(body_size);

    if (message_handler_) {
      message_handler_(std::static_pointer_cast<ProtocolMessage>(raw_message));
    }
  } while(1);
}

//no SharedPtr here, bz of type_cast and don't need guarantee
bool RawProtoService::DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg) {
  CHECK(buffer && out_msg);
  RawMessage* raw_message = static_cast<RawMessage*>(out_msg);

  if (sizeof(RawHeader) == buffer->CanReadSize()) {
    return false;
  }

  const RawHeader* header = (const RawHeader*)buffer->GetRead();
  int32_t frame_size = header->frame_size;

  if (buffer->CanReadSize() < frame_size) {
    return false;
  }

  memcpy(&(raw_message->header_), buffer->GetRead(), frame_size);
  buffer->Consume(frame_size);

  return true;
}

bool RawProtoService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  CHECK(msg && out_buffer);

  const RawMessage* raw_message = static_cast<const RawMessage*>(msg);
  CHECK(raw_message->header_.frame_size == kRawHeaderSize + raw_message->content_.size());

  out_buffer->EnsureWritableSize(raw_message->header_.frame_size);

  out_buffer->WriteRawData(&raw_message->header_, kRawHeaderSize);
  out_buffer->WriteString(raw_message->content_);
#if 0
  VLOG(GLOG_VTRACE) << "Total Size after encode to buffer:" << out_buffer->CanReadSize()
                    << " RawMessage FrameSize is:" << raw_message->header_.frame_size
                    << " Message:\n" << raw_message->MessageDebug();
#endif
  return true;
}

bool RawProtoService::CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) {
  return false;
}

};
