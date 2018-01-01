#ifndef _NET_RAW_PROTOMESSAGE_H_H
#define _NET_RAW_PROTOMESSAGE_H_H

#include <cinttypes>
#include "net_callback.h"

namespace net {

typdef struct {
  uint8_t method;
  uint32_t frame_size;
  uint32_t sequence_id;
} RawHeader;

class RawMessage : public ProtocolMessage {
public:
  RawMessage(IODirectionType t);
  ~RawMessage();

  std::string& MutableContent();
  const std::string& Content() const;

  void SetMethod(uint8_t);
  uint8_t Method() const;
  void SetSequenceId(uint32_t);
  uint32_t SequenceId() const;
  void SetFrameSize(uint32_t);
  uint32_t FrameSize() const;

private:
  std::string content_;
  RawHeader header_;
};

}//end namespace net
#endif
