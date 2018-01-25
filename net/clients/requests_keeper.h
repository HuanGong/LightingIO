#ifndef _LT_NET_REQUESTS_KEPPER_H_
#define _LT_NET_REQUESTS_KEPPER_H_

#include <list>
#include <memory>
#include <net/net_callback.h>
#include <net/protocol/proto_message.h>

namespace net {


class RequestsKeeper {
public:
  RequestsKeeper();
  ~RequestsKeeper();

  RefProtocolMessage& InProgressRequest();

  void PendingRequest(RefProtocolMessage& request);

  RefProtocolMessage PopNextRequest();

  void ResetCurrent();
  void SetCurrent(RefProtocolMessage& request);

  int32_t InQueneCount() const { return requests_.size();}
private:
  RefProtocolMessage current_;
  std::list<RefProtocolMessage> requests_;
};
}//end net
#endif
