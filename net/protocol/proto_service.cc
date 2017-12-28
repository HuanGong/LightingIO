
#include "proto_service.h"
#include "proto_message.h"

namespace net {
ProtoService::ProtoService(const std::string proto) :
  protocol_(proto) {

}
ProtoService::~ProtoService() {};

void ProtoService::SetMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

void ProtoService::InvokeMessageHandler(RefProtocolMessage msg) {
  if (message_handler_) {
    message_handler_(msg);
  }
}

}// end namespace
