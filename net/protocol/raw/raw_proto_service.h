#ifndef NET_RAW_PROTO_SERVICE_H
#define NET_RAW_PROTO_SERVICE_H

#include "raw_message.h"
#include "protocol/proto_service.h"

namespace net {

class RawProtoService : public ProtoService {
public:
  RawProtoService(IODirectionType type);
  ~RawProtoService();

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataRecieved(const RefTcpChannel&, IOBuffer*) override;

  //no SharedPtr here, bz of type_cast and don't need guarantee
  bool DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg) override;
  bool EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) override;
  bool CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) override;
private:
  RefRawMessage current_;
};

}
#endif
