#ifndef NET_COROWORKLOAD_DISPATCHER_H_H
#define NET_COROWORKLOAD_DISPATCHER_H_H

#include "workload_dispatcher.h"

namespace net {

class CoroWlDispatcher : public WorkLoadDispatcher {
public:
  CoroWlDispatcher(bool no_proxy);
  ~CoroWlDispatcher();

  bool Dispatch(ProtoMessageHandler&, RefProtocolMessage&) override;

private:
  void Reply(RefTcpChannel channel, RefProtocolMessage request);
  void DispachToCoroAndReply(ProtoMessageHandler, RefProtocolMessage);
};

}
#endif
