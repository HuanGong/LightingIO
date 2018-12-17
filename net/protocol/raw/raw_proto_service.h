#ifndef NET_RAW_PROTO_SERVICE_H
#define NET_RAW_PROTO_SERVICE_H

#include "raw_message.h"
#include "protocol/proto_service.h"

namespace net {

class RawProtoService : public ProtoService {
public:
  RawProtoService();
  ~RawProtoService();

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataReceived(const RefTcpChannel &, IOBuffer *) override;

  bool CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) override;
  const RefProtocolMessage NewResponseFromRequest(const RefProtocolMessage &req) override;

	void AfterChannelClosed() override;
	void StartHeartBeat(int32_t ms) override;

  bool KeepSequence() override {return false;};

  /* protocol level */
  bool BeforeSendRequest(RawMessage* message);
  bool SendRequestMessage(const RefProtocolMessage &message) override;

  bool ReplyRequest(const RefProtocolMessage& req, const RefProtocolMessage& res) override;
private:
	void OnHeartBeat();
  void HandleMessage(RefRawMessage& message);
  bool heart_beat_alive_ = true;
  base::TimeoutEvent* timeout_ev_ = nullptr;
  static std::atomic<uint64_t> sequence_id_;
};

}
#endif
