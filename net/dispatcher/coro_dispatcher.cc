

#include "glog/logging.h"

#include "tcp_channel.h"
#include "coro_dispatcher.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_task.h"
#include "base/coroutine/coroutine_scheduler.h"

namespace net {

CoroWlDispatcher::CoroWlDispatcher(bool no_proxy)
  : WorkLoadDispatcher(no_proxy) {

}

CoroWlDispatcher::~CoroWlDispatcher() {

}

//IOLoop TO WorkLoop
bool CoroWlDispatcher::Dispatch(ProtoMessageHandler& handler, RefProtocolMessage& message) {
  if (!handler || !message) {
    LOG(ERROR) << "CoroWlDispatcher::Dispatch arguments error";
    return false;
  }
  VLOG(GLOG_VTRACE) << "CoroWlDispatcher Going Dispatch work";
  if (HandleWorkInIOLoop()) {
    DispachToCoroAndReply(handler, message);
    return true;
  }

  base::MessageLoop2* loop = GetNextWorkLoop();
  loop->PostTask(base::NewClosure(std::bind(&CoroWlDispatcher::DispachToCoroAndReply,
                                            this,
                                            handler,
                                            message)));

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
    if (!channel.get()) {
      return;
    }

    if (channel->InIOLoop()) {
      Reply(channel, request);

    } else {
      auto func_reply = std::bind(&CoroWlDispatcher::Reply, this, channel, request);
      channel->IOLoop()->PostTask(base::NewClosure(std::move(func_reply)));
    }
  }
  VLOG(GLOG_VTRACE) << "Request Has Been Handled";
}

// run in io loop
void CoroWlDispatcher::Reply(RefTcpChannel channel, RefProtocolMessage request) {
  CHECK(channel.get());

  RefProtocolMessage response = request->Response();
  if (response.get()) {
    channel->Send(response);
  }
}

}//end namespace
