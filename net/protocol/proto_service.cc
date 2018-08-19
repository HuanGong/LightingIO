
#include "proto_service.h"
#include "proto_message.h"
#include "glog/logging.h"

namespace net {
ProtoService::ProtoService(const std::string proto) :
  protocol_(proto),
  type_(kServer),
  in_message_type_(MessageType::kNone),
  out_message_type_(MessageType::kNone) {
}
ProtoService::~ProtoService() {};

void ProtoService::SetServiceType(ProtocolServiceType t) {
  type_ = t;
  if (type_ == kServer) {
    in_message_type_ = kRequest;
    out_message_type_ = kResponse;
  } else {
    in_message_type_ = kResponse;
    out_message_type_ = kRequest;
  }
}

void ProtoService::SetMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

}// end namespace
