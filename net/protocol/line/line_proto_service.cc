#include "line_proto_service.h"
#include "glog/logging.h"
#include "io_buffer.h"
#include "tcp_channel.h"
#include "line_message.h"

namespace net {

LineProtoService::LineProtoService()
  : ProtoService("line") {
}

LineProtoService::~LineProtoService() {
}

void LineProtoService::OnStatusChanged(const RefTcpChannel& channel) {
  ;
}
void LineProtoService::OnDataFinishSend(const RefTcpChannel& channel) {
  ;
}

void LineProtoService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buf) {
  const uint8_t* line_crlf =  buf->FindCRLF();
  if (!line_crlf) {
    return;
  }

  {
    IODirectionType type = channel->IsServerChannel() ?
      IODirectionType::kInRequest : IODirectionType::kInReponse;

    std::shared_ptr<LineMessage> msg = std::make_shared<LineMessage>(type);
    //IOCtx
    msg->SetIOContextWeakChannel(channel);

    const uint8_t* start = buf->GetRead();
    int32_t len = line_crlf - start;

    std::string& body = msg->MutableBody();
    body.assign((const char*)start, len);

    buf->Consume(len + 2/*lenth of /r/n*/);

    if (message_handler_) {
      message_handler_(std::static_pointer_cast<ProtocolMessage>(msg));
    }
  }
}

bool LineProtoService::DecodeToMessage(IOBuffer* buf, ProtocolMessage* out_msg) {

  LineMessage* line_msg = static_cast<LineMessage*>(out_msg);

  const uint8_t* line_crlf =  buf->FindCRLF();
  if (!line_crlf) {
    return false;
  }
  const uint8_t* start = buf->GetRead();
  int32_t len = line_crlf - start;

  std::string& body = line_msg->MutableBody();
  body.assign((const char*)start, len);

  buf->Consume(len + 2/*lenth of /r/n*/);
  return true;
}

bool LineProtoService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  if (msg->Protocol() != "line") {
    return false;
  }
  const LineMessage* line_msg = static_cast<const LineMessage*>(msg);
  out_buffer->EnsureWritableSize(line_msg->Body().size() + 2);
  out_buffer->WriteString(line_msg->Body());
  out_buffer->WriteRawData("\r\n", 2);
  return true;
}

bool LineProtoService::CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) {
  return false;
}

}//end of file
