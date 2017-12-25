#ifndef NET_PROTOCOL_MESSAGE_H
#define NET_PROTOCOL_MESSAGE_H
namespace net {

class ProtocolMessage {
public:
  virtual ~ProtocolMessage();

  /*encode protocol msg(http redis raw etc) to ByteBuffer*/
  virtual bool Encode() {};
  /*decode ByteBuffer to special protocol,http redis raw etc*/
  virtual bool Decode() {};
private:
};

}
#endif
