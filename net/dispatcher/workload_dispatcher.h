#ifndef NET_WORKLOAD_DISPATCHER_H
#define NET_WORKLOAD_DISPATCHER_H

#include "net_callback.h"

namespace net {

class WorkLoadDispatcher {
public:
  WorkLoadDispatcher() {};
  virtual ~WorkLoadDispatcher() {};

  virtual bool Dispatch(ProtoMessageHandler&, RefProtocolMessage&) = 0;

};

}//end namespace net
#endif
