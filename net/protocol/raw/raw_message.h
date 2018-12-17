#ifndef _NET_RAW_PROTOMESSAGE_H_H
#define _NET_RAW_PROTOMESSAGE_H_H

#include <cinttypes>
#include "net_callback.h"
#include "protocol/proto_message.h"

namespace net {

typedef struct _HT {
  _HT()
    : code(0),
      method(0),
      frame_size(0),
      sequence_id(0) {
  }
  /* use for status code*/
  uint8_t code;
  /* Method of this message*/
  uint8_t method;
  /* Size of Network Frame: frame_size = sizeof(RawHeader) + Len(content_)*/
  uint32_t frame_size;
  /* raw message sequence, for better asyc request sequence */
  uint64_t sequence_id;
} RawHeader;

class RawMessage;
typedef std::shared_ptr<RawMessage> RefRawMessage;

class RawMessage : public ProtocolMessage {
public:
  typedef RawMessage ResponseType;

  static const uint32_t kRawHeaderSize;
  static RefRawMessage CreateRequest();
  static RefRawMessage CreateResponse();

  RawMessage(MessageType t);
  ~RawMessage();

  void SetCode(uint8_t code);
  void SetMethod(uint8_t method);
  const std::string& Content() const;
  void SetContent(const char* content);
  void SetContent(const std::string& body);
  const std::string Dump() const override;

  uint8_t Method() const {return header_.method;}
  const RawHeader& Header() const {return header_;}
  const uint64_t Identify() const {return header_.sequence_id;}
private:
  friend class RawProtoService;
  void CalculateFrameSize();
  std::string& MutableContent() {return content_;}
  uint64_t SequenceId() const {return header_.sequence_id;}
  void SetSequenceId(uint64_t id) {header_.sequence_id = id;}
  RawHeader header_;
  std::string content_;
};

}//end namespace net
#endif
