#include "requests_keeper.h"

#include <net/tcp_channel.h>

namespace net {
const static RefProtocolMessage kNullRequest;

RequestsKeeper::RequestsKeeper() {
}

RequestsKeeper::~RequestsKeeper() {
}

RefProtocolMessage& RequestsKeeper::InProgressRequest() {
  return current_;
}

void RequestsKeeper::PendingRequest(RefProtocolMessage& request) {
  requests_.push_back(request);
}

RefProtocolMessage RequestsKeeper::PopNextRequest() {
  if (0 == InQueneCount()) {
    return kNullRequest;
  }
  RefProtocolMessage next = requests_.front();
  requests_.pop_front();
  return next;
}

void RequestsKeeper::ResetCurrent() {
  current_.reset();
}

void RequestsKeeper::SetCurrent(RefProtocolMessage& request) {
  current_ = request;
}


}//end net
