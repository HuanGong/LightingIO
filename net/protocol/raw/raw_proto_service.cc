
#include <memory>
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

    auto raw_message = std::make_shared<RawMessage>();

    raw_message->SetMessageType(InMessageType());
    raw_message->SetIOContextWeakChannel(channel);

    memcpy(&(raw_message->header_), buffer->GetRead(), kRawHeaderSize);
    buffer->Consume(kRawHeaderSize);

    std::string& content = raw_message->MutableContent();
    content.append((const char*)buffer->GetRead(), body_size);
    buffer->Consume(body_size);
    LOG(INFO) << " protocol service " << InMessageType() << " got sequence:" << raw_message->header_.sequence_id;
    if (message_handler_) {
      message_handler_(std::static_pointer_cast<ProtocolMessage>(raw_message));
    }
  } while(1);
}

bool RawProtoService::EnsureProtocol(ProtocolMessage* message)  {
  CHECK(ServiceType() == ProtocolServiceType::kClient);
  RawMessage* request = static_cast<RawMessage*>(message);
  request->SetSequenceId(sequence_id_);
  sequence_id_++;
  if (sequence_id_ == 0) {
    sequence_id_++;
  }
  return true;
}

bool RawProtoService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  CHECK(msg && out_buffer);

  const RawMessage* raw_message = static_cast<const RawMessage*>(msg);
  CHECK(raw_message->header_.frame_size == kRawHeaderSize + raw_message->content_.size());
  LOG(INFO) << __FUNCTION__ << " encode identify:" << raw_message->header_.sequence_id;

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

void RawProtoService::BeforeReplyMessage(ProtocolMessage* in, ProtocolMessage* out) {
  RawMessage* raw_response = (RawMessage*)out;
  RawMessage* raw_request = (RawMessage*)in;
  LOG(INFO) << "raw server replay request id:" << raw_request->Header().sequence_id;
  raw_response->SetSequenceId(raw_request->Header().sequence_id);
}

void RawProtoService::BeforeSendMessage(ProtocolMessage* message)  {

}

bool RawProtoService::CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) {
  return false;
}

const RefProtocolMessage RawProtoService::DefaultResponse(const RefProtocolMessage& req) {
  return std::make_shared<RawMessage>();
}

};
