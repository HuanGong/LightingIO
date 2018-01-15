
#include "client_channel.h"

#include "base/base_constants.h"

namespace net {


RefClientChannel ClientChannel::Create(RefTcpChannel& channel) {
  return std::make_shared<ClientChannel>(channel);
}

ClientChannel::ClientChannel(RefTcpChannel& channel)
  : channel_(channel) {
}

ClientChannel::~ClientChannel() {

}

bool ClientChannel::StartRequest(RefProtocolMessage& request) {
  if (!channel_->IsConnected()) {
    VLOG(GLOG_VERROR) << "StartRequest Failed, Channel Has Broken";
    return false;
  }
  return channel_->SendProtoMessage(request);
}

}//end net
