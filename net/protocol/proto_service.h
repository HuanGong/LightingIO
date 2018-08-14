#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "../net_callback.h"
#include "proto_message.h"
#include "dispatcher/workload_dispatcher.h"

namespace net {

typedef enum {
  kServer = 0,
  kClient = 1
} ProtocolServiceType;

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

  //Before send [request type] message, in normal case, this was used for
  //async clients request
  virtual void BeforeSendMessage(ProtocolMessage* out_message) {};
  virtual void BeforeReplyMessage(ProtocolMessage* in, ProtocolMessage* out) {};

  void SetServiceType(ProtocolServiceType t);
  inline const std::string& Protocol() const {return protocol_;};
  inline ProtocolServiceType ServiceType() const {return type_;}
  inline MessageType InMessageType() const {return in_message_type_;};
  inline MessageType OutMessageType() const {return out_message_type_;};

  virtual const RefProtocolMessage DefaultResponse(const RefProtocolMessage&) {return NULL;}
  virtual bool CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response);
protected:
  std::string protocol_;
  ProtocolServiceType type_;
  MessageType in_message_type_;
  MessageType out_message_type_;
  WorkLoadDispatcher* dispatcher_;
  ProtoMessageHandler message_handler_;
};

}
#endif
