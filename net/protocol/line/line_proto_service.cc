#include "line_proto_service.h"
#include "glog/logging.h"
#include "io_buffer.h"
#include "tcp_channel.h"

namespace net {

LineProtoService::LineProtoService()
  : ProtoService("line") {
}

LineProtoService::~LineProtoService() {
}

void LineProtoService::OnStatusChanged(const RefTcpChannel& channel) {
  LOG(INFO) << "RefTcpChannel status changed:" << channel->StatusAsString();
}
void LineProtoService::OnDataFinishSend(const RefTcpChannel& channel) {
  LOG(INFO) << "RefTcpChannel status changed:" << channel->StatusAsString();
}

void LineProtoService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buf) {
  const uint8_t* line_crlf =  buf->FindCRLF();
  if (!line_crlf) {
    return;
  }
  const uint8_t* start = buf->GetRead();
  int32_t len = line_crlf - start;
  std::string line((const char*)start, len);
  buf->Consume(len + 2/*lenth of /r/n*/);
}


}//end of file
