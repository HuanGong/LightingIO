#ifndef PROTOCOL_MESSAGE_H_
#define PROTOCOL_MESSAGE_H_

#include <string>

namespace net {

class ProtocolMessage {
public:
  virtual ~ProtocolMessage();

  virtual bool DumpToString(std::string* out);
  virtual bool DecodeContent() {};
  virtual bool EncodeContent() {};
private:

};

}//end net
#endif
