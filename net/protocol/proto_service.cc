
#include "proto_service.h"
#include "proto_message.h"
#include "glog/logging.h"

namespace net {
ProtoService::ProtoService(const std::string proto) :
  protocol_(proto),
  dispatcher_(NULL) {

}
ProtoService::~ProtoService() {};

void ProtoService::SetMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

void ProtoService::SetMessageDispatcher(WorkLoadDispatcher* wld) {
  dispatcher_ = wld;
}

bool ProtoService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  return false;
}
bool ProtoService::DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg) {
  return false;
}

bool ProtoService::CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) {
  return true;
}

}// end namespace
