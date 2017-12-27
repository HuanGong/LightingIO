#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "../net_callback.h"

namespace net {
/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService {
public:
  ProtoService(const std::string proto) :
    protocol_(proto) {
  };
  virtual ~ProtoService() {};

  virtual void SetMessageHandler(ProtoMessageHandler) = 0;

  virtual void OnStatusChanged(const RefTcpChannel&) = 0;
  virtual void OnDataFinishSend(const RefTcpChannel&) = 0;
  virtual void OnDataRecieved(const RefTcpChannel&, IOBuffer*) = 0;

  const std::string& Protocol() {return protocol_;};
protected:
  //void HandleMessage();
  std::string protocol_;

};

}
#endif
