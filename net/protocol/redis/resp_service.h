#ifndef _NET_PROTOCOL_RESP_SERVICE_H_H
#define _NET_PROTOCOL_RESP_SERVICE_H_H

#include "redis_response.h"
#include "protocol/proto_service.h"

namespace net {

class RespService : public ProtoService {
public:
  RespService();
  ~RespService();

  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;

  void OnDataRecieved(const RefTcpChannel&, IOBuffer*) override;
  bool EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) override;

  void BeforeSendMessage(ProtocolMessage* out_message) override;
  void BeforeReplyMessage(ProtocolMessage* in, ProtocolMessage* out) override;
private:
  uint32_t next_incoming_count_ = 0;
  RefRedisResponse current_response;// = std::make_shared<RedisResponse>();
  resp::decoder decoder_;
};

}// end namespace
#endif
