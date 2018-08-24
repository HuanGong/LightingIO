#include "async_channel.h"
#include "base/base_constants.h"
#include "protocol/proto_service.h"

namespace net {

const static RefProtocolMessage kNullResponse;

AsyncChannel::AsyncChannel(Delegate* d, RefTcpChannel& ch)
  : ClientChannel(d, ch) {
}

AsyncChannel::~AsyncChannel() {
}

void AsyncChannel::StartClient() {
  ProtoService* service = channel_->GetProtoService();

  ProtoMessageHandler res_handler = std::bind(&AsyncChannel::OnResponseMessage,
                                             this, std::placeholders::_1);
  service->SetMessageHandler(std::move(res_handler));

  ChannelClosedCallback close_callback = std::bind(&AsyncChannel::OnChannelClosed,
                                                   this, std::placeholders::_1);
  channel_->SetCloseCallback(close_callback);

  channel_->Start();
}

void AsyncChannel::SendRequest(RefProtocolMessage request)  {
  CHECK(channel_->InIOLoop());
  LOG(INFO) << __FUNCTION__ << " enter";

  bool success = channel_->GetProtoService()->EnsureProtocol(request.get());

  if (success) {
    LOG(INFO) << __FUNCTION__ << " call channel sendprotomessage";
    success = channel_->SendProtoMessage(request);
  }

  if (!success) {
    request->SetFailInfo(FailInfo::kChannelBroken);
    delegate_->OnRequestGetResponse(request, kNullResponse);
    return;
  }

  LOG(INFO) << __FUNCTION__ << " schedule tiemout task";
  WeakProtocolMessage weak(request); // weak ptr must init outside, Take Care of weakptr
  auto functor = std::bind(&AsyncChannel::OnRequestTimeout, shared_from_this(), weak);
  IOLoop()->PostDelayTask(base::NewClosure(functor), request_timeout_);

  uint64_t message_identify = request->MessageIdentify();
  CHECK(message_identify !=MessageIdentifyType::KInvalidIdentify);
  LOG(INFO) << "Send Request identify:" << message_identify;
  in_progress_.insert(std::make_pair(message_identify, std::move(request)));
  LOG(INFO) << __FUNCTION__ << " leave";
}

void AsyncChannel::OnRequestTimeout(WeakProtocolMessage weak) {

  RefProtocolMessage request = weak.lock();
  if (!request || request->IsResponsed()) {
    return;
  }

  uint64_t identify = request->MessageIdentify();
  CHECK(identify != MessageIdentifyType::KInvalidIdentify);

  size_t numbers = in_progress_.erase(identify);
  if (numbers == 0) {
    return;
  }

  request->SetFailInfo(FailInfo::kTimeOut);
  delegate_->OnRequestGetResponse(request, kNullResponse);
}

void AsyncChannel::OnResponseMessage(const RefProtocolMessage& res) {
  CHECK(channel_->InIOLoop());

  uint64_t identify = res->MessageIdentify();
  CHECK(identify != MessageIdentifyType::KInvalidIdentify);

  auto iter = in_progress_.find(identify);
  if (iter == in_progress_.end()) {
    return;
  }
  for (auto kv : in_progress_) {
    LOG(INFO) << " key identify:" << kv.first << " message identify:" << kv.second->MessageIdentify();
  }
  LOG(INFO) << __FUNCTION__ <<  " " << iter->second->MessageIdentify() << " response identify:" << identify;
  CHECK(iter->second->MessageIdentify() == identify);

  delegate_->OnRequestGetResponse(iter->second, res);
  in_progress_.erase(iter);
}

void AsyncChannel::OnChannelClosed(const RefTcpChannel& channel) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " , Channel:" << channel_->ChannelName();

  for (auto kv : in_progress_) {
    kv.second->SetFailInfo(FailInfo::kChannelBroken);
    delegate_->OnRequestGetResponse(kv.second, kNullResponse);
  }
  in_progress_.clear();
  auto guard = shared_from_this();
  delegate_->OnClientChannelClosed(guard);
}

}//net

