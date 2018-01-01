#ifndef NET_COROWORKLOAD_DISPATCHER_H_H
#define NET_COROWORKLOAD_DISPATCHER_H_H

#include "workload_dispatcher.h"

namespace net {

class CoroWlDispatcher : public WorkLoadDispatcher {
public:
  CoroWlDispatcher();
  ~CoroWlDispatcher();

  bool Dispatch(ProtoMessageHandler&, RefProtocolMessage&) override;

private:
  void DispachToCoroAndReply(ProtoMessageHandler, RefProtocolMessage);
};

}
#endif
