#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "../net_callback.h"
#include "dispatcher/workload_dispatcher.h"

namespace net {
/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService {
public:
  ProtoService(const std::string proto);
  virtual ~ProtoService();

  void SetMessageHandler(ProtoMessageHandler);
  void SetMessageDispatcher(WorkLoadDispatcher*);

  virtual void OnStatusChanged(const RefTcpChannel&) = 0;
  virtual void OnDataFinishSend(const RefTcpChannel&) = 0;
  virtual void OnDataRecieved(const RefTcpChannel&, IOBuffer*) = 0;

  virtual bool EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer);
  virtual bool DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg);
  const std::string& Protocol() {return protocol_;};

  virtual const RefProtocolMessage DefaultResponse(const RefProtocolMessage&) {return NULL;}
  virtual bool CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response);
protected:
  bool InvokeMessageHandler(RefProtocolMessage);
  //void HandleMessage();
  std::string protocol_;
  WorkLoadDispatcher* dispatcher_;
  ProtoMessageHandler message_handler_;
};

}
#endif
