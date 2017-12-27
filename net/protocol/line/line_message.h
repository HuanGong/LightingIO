#ifndef _NET_LINE_PROTO_MESSAGE_H
#define _NET_LINE_PROTO_MESSAGE_H

#include "net/proto_message.h"

namespace net {
class LineMessage : public ProtocolMessage {
public:
  LineMessage(ProtoMsgType t);

  std::string& MutableBody();
  const std::string& Body() const;
private:
  std::string body_;
};

}
#endif
