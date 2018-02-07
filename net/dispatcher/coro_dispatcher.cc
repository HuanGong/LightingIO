

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

//for client out-request and response-back
bool CoroWlDispatcher::PrepareOutRequestContext(RefProtocolMessage& message) {
  if (NULL == base::MessageLoop2::Current() ||
      !base::CoroScheduler::TlsCurrent()->CurrentCoro() ||
      base::CoroScheduler::TlsCurrent()->InRootCoroutine()) {
    return false;
  }

  auto& work_context = message->GetWorkCtx();
  work_context.coro_loop = base::MessageLoop2::Current();
  work_context.weak_coro = base::CoroScheduler::TlsCurrent()->CurrentCoro();
  return true;
}

void CoroWlDispatcher::TransferAndYield(base::MessageLoop2* ioloop, base::StlClourse clourse) {
  CHECK(ioloop);
  CHECK(base::CoroScheduler::TlsCurrent()->CurrentCoro());
  ioloop->PostTask(base::NewClosure(std::move(clourse)));
  base::CoroScheduler::TlsCurrent()->YieldCurrent();
}

bool CoroWlDispatcher::ResumeWorkCtxForRequest(RefProtocolMessage& request) {

  auto& work_context = request->GetWorkCtx();
  if (!work_context.coro_loop || !work_context.weak_coro.lock()) {
    LOG(FATAL) << " This Request WorkContext Is Not Init";
    return false;
  }

  base::StlClourse functor = [=]() {
    base::RefCoroutine coro = work_context.weak_coro.lock();
    if (coro && coro->Status() == base::CoroState::kPaused) {
      base::CoroScheduler::TlsCurrent()->ResumeCoroutine(coro);
    }
  };

  work_context.coro_loop->PostTask(base::NewClosure(std::move(functor)));
  return true;
}

bool CoroWlDispatcher::Play(base::StlClourse& clourse) {
  if (HandleWorkInIOLoop()) {
    base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(std::move(clourse)));
    return true;
  }

  base::MessageLoop2* loop = GetNextWorkLoop();
  if (NULL == loop) {
    LOG(ERROR) << "NO WorkLoop Handle this Request Clourse";
    return false;
  }
  auto functor = [=]() {
    base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(std::move(clourse)));
  };
  loop->PostTask(base::NewClosure(functor));
  return true;
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
  } else {

    base::MessageLoop2* loop = GetNextWorkLoop();
    loop->PostTask(base::NewClosure(std::bind(&CoroWlDispatcher::DispachToCoroAndReply,
                                              this,
                                              handler,
                                              message)));

  }
  return true;
}

//Handle Work in WorkLoop and the Response This Request In WorkLoop
void CoroWlDispatcher::DispachToCoroAndReply(ProtoMessageHandler functor,
                                             RefProtocolMessage request) {

  functor(request);

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
  //VLOG(GLOG_VTRACE) << "Request Has Been Handled";
}

// run in io loop
void CoroWlDispatcher::Reply(RefTcpChannel channel, RefProtocolMessage request) {
  CHECK(channel.get());
  if (!channel.get()) {
    LOG(INFO) << "A Bad Channel Can't Send Response";
    return;
  }

  RefProtocolMessage response = request->Response();
  if (response.get()) {
    channel->SendProtoMessage(response);
  }
}

void CoroWlDispatcher::SetWorkContext(ProtocolMessage* message) {
  CHECK(message);
  auto& work_context = message->GetWorkCtx();
  work_context.coro_loop = base::MessageLoop2::Current();
  work_context.weak_coro = base::CoroScheduler::TlsCurrent()->CurrentCoro();
};

bool CoroWlDispatcher::TransmitToWorker(base::StlClourse& closure) {

  if (HandleWorkInIOLoop()) {
    base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(std::move(closure)));
    return true;
  }

  base::MessageLoop2* loop = GetNextWorkLoop();
  if (loop) {

    auto coro_functor = [=]() {
      base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(std::move(closure)));
    };

    loop->PostTask(base::NewClosure(coro_functor));
    return true;
  }
  return false;
}

}//end namespace
