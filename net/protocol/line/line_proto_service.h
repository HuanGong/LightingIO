#ifndef NET_LINE_PROTO_SERVICE_H
#define NET_LINE_PROTO_SERVICE_H

#include "net/proto_service.h"

namespace net {

class LineProtoService : public ProtoService {
public:
  LineProtoService();
  ~LineProtoService();

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void SetMessageHandler(ProtoMessageHandler) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataRecieved(const RefTcpChannel&, IOBuffer*) override;
protected:

private:
  ProtoMessageHandler message_handler_;
};

}
#endif
