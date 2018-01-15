#ifndef _LT_NET_REQUESTS_KEPPER_H_
#define _LT_NET_REQUESTS_KEPPER_H_

#include <list>
#include <net/net_callback.h>
#include <net/protocol/proto_message.h>

namespace net {

class RequestsKeeper {
public:
  RequestsKeeper();
  ~RequestsKeeper();

  bool CanLaunchNext();

  RefProtocolMessage& InProgressRequest();

  void PendingRequest(RefProtocolMessage& request);
private:
  RefProtocolMessage current_;
  std::list<RefProtocolMessage> requests_;
};
}//end net
#endif
