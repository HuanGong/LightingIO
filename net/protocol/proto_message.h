#ifndef NET_PROTOCOL_MESSAGE_H
#define NET_PROTOCOL_MESSAGE_H

#include <string>

namespace net {

typedef enum {
  kUndefined = 0,
  kInRequest = 1,
  kInReponse = 2,
  kOutRequest = 3,
  kOutResponse = 4
} ProtoMsgType;

class ProtocolMessage {
public:
  ProtocolMessage(const std::string protocol);
  virtual ~ProtocolMessage();

  const std::string& Protocol() const;

  void SetMessageType(ProtoMsgType t);
  const ProtoMsgType MessageType() const;
private:
  std::string proto_;
  ProtoMsgType type_;
};

}
#endif
