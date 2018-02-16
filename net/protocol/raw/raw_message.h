#ifndef _NET_RAW_PROTOMESSAGE_H_H
#define _NET_RAW_PROTOMESSAGE_H_H

#include <cinttypes>
#include "net_callback.h"
#include "protocol/proto_message.h"

namespace net {

typedef struct {
  /* use for status code*/
  uint8_t code;
  /* Method of this message*/
  uint8_t method;
  /* Size of Network Frame: frame_size = sizeof(RawHeader) + Len(content_)*/
  uint32_t frame_size;
  /* raw message sequence, for better asyc request sequence */
  uint32_t sequence_id;
} RawHeader;

class RawMessage;
typedef std::shared_ptr<RawMessage> RefRawMessage;

class RawMessage : public ProtocolMessage {
public:
  RawMessage(IODirectionType t);
  ~RawMessage();

  std::string& MutableContent();
  const std::string& Content() const;

  const RawHeader& Header();
  void SetCode(uint8_t code);
  void SetMethod(uint8_t method);
  void SetFrameSize(uint32_t frame_size);
  void SetSequenceId(uint32_t sequence_id);
private:
  friend class RawProtoService;

  RawHeader header_;
  std::string content_;
};

}//end namespace net
#endif
