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
};

}
#endif
