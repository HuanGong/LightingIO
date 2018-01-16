
#include "client_channel.h"

#include "base/base_constants.h"

namespace net {

RefClientChannel ClientChannel::Create(RefTcpChannel& channel) {
  return std::make_shared<ClientChannel>(channel);
}

ClientChannel::ClientChannel(RefTcpChannel& channel)
  : channel_(channel) {

  channel_->SetCloseCallback(std::bind(&ClientChannel::OnChannelClosed,
                                       this, std::placeholders::_1));
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

void ClientChannel::OnChannelClosed(RefTcpChannel channel) {
  VLOG(GLOG_VTRACE) << "Client's TcpChannel Closed";
}

void ClientChannel::OnResponseMessage(RefProtocolMessage message) {
  LOG(INFO) << "ClientChannel Recv A Response Message";

}

void ClientChannel::SetResponseHandler(ResponseHandler handler) {
  response_handler_ = handler;
}

}//end net
