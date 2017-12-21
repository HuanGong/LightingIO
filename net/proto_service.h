
#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "net_callback.h"
namespace net {
/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService {
public:
  virtual ~ProtoService();

  virtual void OnStatusChanged(const RefTcpChannel&);
  virtual void OnDataFinishSend(const RefTcpChannel&);
  virtual void OnDataRecieved(const RefTcpChannel&, IOBuffer*);
};

}
#endif
