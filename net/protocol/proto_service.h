#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "../net_callback.h"
#include "proto_message.h"

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

  virtual void OnStatusChanged(const RefTcpChannel&) = 0;
  virtual void OnDataFinishSend(const RefTcpChannel&) = 0;

  virtual void OnDataRecieved(const RefTcpChannel&, IOBuffer*) = 0;
  virtual bool EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) = 0;
  //Before send [request type] message, in normal case, this was used for
  //async clients request
  virtual bool KeepSequence() {return true;};
  virtual bool EnsureProtocol(ProtocolMessage* message) {return true;};
  virtual void BeforeSendMessage(ProtocolMessage* out_message) {};
  virtual void BeforeReplyMessage(ProtocolMessage* in, ProtocolMessage* out) {};
  virtual bool CloseAfterMessage(ProtocolMessage*, ProtocolMessage*) { return true;};
  virtual const RefProtocolMessage DefaultResponse(const RefProtocolMessage&) {return NULL;}

  void SetServiceType(ProtocolServiceType t);
  inline const std::string& Protocol() const {return protocol_;};
  inline ProtocolServiceType ServiceType() const {return type_;}
  inline MessageType InMessageType() const {return in_message_type_;};
  inline MessageType OutMessageType() const {return out_message_type_;};
protected:
  std::string protocol_;
  ProtocolServiceType type_;
  MessageType in_message_type_;
  MessageType out_message_type_;
  ProtoMessageHandler message_handler_;
};

}
#endif
