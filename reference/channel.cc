#include "channel.h"

namespace net {

ClientChannel::ClientChannel(ChannelInfo channel_info) :
  timeout_ms_(1000),
  auto_reconnection_(false),
  reconnection_interval_(0),
  status_(NOT_EXSIST),
  channel_info_(channel_info) {
}

ClientChannel::~ClientChannel() {
}

void ClientChannel::SetTimeout(uint32_t timeout_ms) {
  timeout_ms_ = timeout_ms;
}
void ClientChannel::SetAutoReconnection(bool auto_reconnect) {
  auto_reconnection_ = auto_reconnect;
}
void ClientChannel::SetReconnectioninterval(uint32_t interval) {
  reconnection_interval_ = interval;
}


}//endnamespace
