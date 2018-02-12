#ifndef NET_LINE_PROTO_SERVICE_H
#define NET_LINE_PROTO_SERVICE_H

#include "protocol/proto_service.h"

namespace net {

class LineProtoService : public ProtoService {
public:
  LineProtoService();
  ~LineProtoService();

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataRecieved(const RefTcpChannel&, IOBuffer*) override;

  //no SharedPtr here, bz of type_cast and don't need guarantee
  //it's lifetime in this contex
  bool DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg) override;
  bool EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) override;
  bool CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) override;
};

}
#endif
