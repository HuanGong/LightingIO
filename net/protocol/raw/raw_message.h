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
  uint32_t sequence_id;
} RawHeader;

class RawMessage;
typedef std::shared_ptr<RawMessage> RefRawMessage;

class RawMessage : public ProtocolMessage {
public:
  static const uint32_t kRawHeaderSize;
  static RefRawMessage CreateRequest();
  static RefRawMessage CreateResponse();

  RawMessage();
  ~RawMessage();

  void SetCode(uint8_t code);
  void SetMethod(uint8_t method);
  const std::string& Content() const;
  void SetContent(const char* content);
  void SetContent(const std::string& body);
  const RawHeader& Header() const {return header_;}
  const std::string MessageDebug() const override;
private:
  friend class RawProtoService;
  void SetSequenceId(uint32_t sequence_id);
  std::string& MutableContent() {return content_;}

  RawHeader header_;
  std::string content_;
};

}//end namespace net
#endif
