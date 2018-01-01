

#include "glog/logging.h"

#include "tcp_channel.h"
#include "coro_dispatcher.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"

#include "base/coroutine/coroutine_task.h"
#include "base/coroutine/coroutine_scheduler.h"

namespace net {

CoroWlDispatcher::CoroWlDispatcher()
  : WorkLoadDispatcher() {

}

CoroWlDispatcher::~CoroWlDispatcher() {

}

//IOLoop TO WorkLoop
bool CoroWlDispatcher::Dispatch(ProtoMessageHandler& h, RefProtocolMessage& m) {

  LOG(INFO) << "Going Dispatch work";
  if (!h || !m) {
    return false;
  }
  //TODO: logic for dispatch things to difference workloop handle
  DispachToCoroAndReply(h, m);
  return true;
}

//Handle Work in WorkLoop and the Response This Request In WorkLoop
void CoroWlDispatcher::DispachToCoroAndReply(ProtoMessageHandler functor,
                                             RefProtocolMessage request) {

  auto coro_functor = std::bind(functor, request);
  base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(std::move(coro_functor)));

  RefProtocolMessage response = request->Response();
  if (response.get()) {
    WeakPtrTcpChannel weak_channel = request->GetIOCtx().channel;
    RefTcpChannel channel = weak_channel.lock();
    if (channel) {
      LOG(INFO) << "Send A Response to Client";
      channel->Send(response);
    }
  }
  LOG(INFO) << "Dispatch Finished work be done";
}


}//end namespace
