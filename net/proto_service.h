
#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "net_callback.h"
/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService {
public:
  virtual ~ProtoService();

  virtual void OnStatusChanged(const RefConnectionChannel&);
  virtual void OnConnectionClosed(const RefConnectionChannel&);
  virtual void OnDataFinishSend(const RefConnectionChannel&)
  virtual void OnDataRecieved(const RefConnectionChannel&, IOBuffer*);

};

#endif
