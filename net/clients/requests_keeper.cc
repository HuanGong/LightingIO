#include "requests_keeper.h"

#include <net/tcp_channel.h>

namespace net {

RequestsKeeper::RequestsKeeper() {
}

RequestsKeeper::~RequestsKeeper() {
}

bool RequestsKeeper::CanLaunchNext() {
  return (current_.get() == NULL);
}

RefProtocolMessage& RequestsKeeper::InProgressRequest() {
  return current_;
}

void RequestsKeeper::PendingRequest(RefProtocolMessage& request) {
  requests_.push_front(request);
}

}//end net
