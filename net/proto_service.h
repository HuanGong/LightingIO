#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "net_callback.h"

namespace net {
/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService {
public:
  ProtoService() { };
  virtual ~ProtoService() {};

  virtual void OnStatusChanged(const RefTcpChannel&) = 0;
  virtual void OnDataFinishSend(const RefTcpChannel&) = 0;
  virtual void OnDataRecieved(const RefTcpChannel&, IOBuffer*) = 0;
};

}
#endif
