#include "async_channel.h"
#include "queued_channel.h"
#include "base/base_constants.h"
#include "protocol/proto_service.h"

namespace net {

const static RefProtocolMessage kNullResponse;

RefClientChannel CreateClientChannel(ClientChannel::Delegate* delegate, RefTcpChannel& channel) {
  if (channel->GetProtoService()->KeepSequence()) {
    auto client_channel = std::make_shared<QueuedChannel>(delegate, channel);
    return std::static_pointer_cast<ClientChannel>(client_channel);
  }
  auto client_channel = std::make_shared<AsyncChannel>(delegate, channel);
  return std::static_pointer_cast<ClientChannel>(client_channel);
}

QueuedChannel::QueuedChannel(Delegate* d, RefTcpChannel& ch)
  : ClientChannel(d, ch) {
}

QueuedChannel::~QueuedChannel() {
}

void QueuedChannel::StartClient() {
  ProtoService* service = channel_->GetProtoService();

  ProtoMessageHandler res_handler = std::bind(&QueuedChannel::OnResponseMessage,
                                             this, std::placeholders::_1);
  service->SetMessageHandler(std::move(res_handler));

  ChannelClosedCallback close_callback = std::bind(&QueuedChannel::OnChannelClosed,
                                                   this, std::placeholders::_1);
  channel_->SetCloseCallback(close_callback);

  channel_->Start();
}

void QueuedChannel::SendRequest(RefProtocolMessage request)  {
  CHECK(channel_->InIOLoop());

  waiting_list_.push_back(std::move(request));

  TrySendRequestInternal();
}

bool QueuedChannel::TrySendRequestInternal() {
  if (in_progress_request_) {
    return false;
  }

  bool success = false;
  while(!success && waiting_list_.size()) {

    RefProtocolMessage& next = waiting_list_.front();
    success = channel_->SendProtoMessage(next);
    waiting_list_.pop_front();
    if (success) {
      in_progress_request_ = next;
    } else {
      next->SetFailInfo(FailInfo::kChannelBroken);
      LOG(INFO) << __FUNCTION__ << " send failed call OnrequestGetResponse";
      delegate_->OnRequestGetResponse(next, kNullResponse);
    }
  }

  if (success) {
    CHECK(in_progress_request_);
    WeakProtocolMessage weak(in_progress_request_);   // weak ptr must init outside, Take Care of weakptr
    auto functor = std::bind(&QueuedChannel::OnRequestTimeout, shared_from_this(), weak);
    IOLoop()->PostDelayTask(NewClosure(functor), request_timeout_);
  }
  return success;
}

void QueuedChannel::OnRequestTimeout(WeakProtocolMessage weak) {

  RefProtocolMessage request = weak.lock();
  if (!request) return;

  if (request.get() != in_progress_request_.get()) {
    return;
  }
  LOG(INFO) << __FUNCTION__ << " call OnrequestGetResponse";
  request->SetFailInfo(FailInfo::kTimeOut);
  delegate_->OnRequestGetResponse(request, kNullResponse);
  in_progress_request_.reset();

  if (channel_->IsConnected()) {
    channel_->ShutdownChannel();
  } else {
    OnChannelClosed(channel_);
  }
}

void QueuedChannel::OnResponseMessage(const RefProtocolMessage& res) {
  CHECK(channel_->InIOLoop());

  if (!in_progress_request_) {
    LOG(ERROR) << "Got Response Without Request or Request Has Canceled";
    return ;
  }

  LOG(INFO) << __FUNCTION__ << " call OnrequestGetResponse";
  delegate_->OnRequestGetResponse(in_progress_request_, res);
  in_progress_request_.reset();

  TrySendRequestInternal();
}

void QueuedChannel::OnChannelClosed(const RefTcpChannel& channel) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " , Channel:" << channel_->ChannelName();

  if (in_progress_request_) {
    in_progress_request_->SetFailInfo(FailInfo::kChannelBroken);
    LOG(INFO) << __FUNCTION__ << " reset inprogress request call OnrequestGetResponse";
    delegate_->OnRequestGetResponse(in_progress_request_, kNullResponse);
    in_progress_request_.reset();
  }

  while(waiting_list_.size()) {
    RefProtocolMessage& request = waiting_list_.front();
    request->SetFailInfo(FailInfo::kChannelBroken);
    LOG(INFO) << __FUNCTION__ << " reset waitlist request call OnrequestGetResponse";
    delegate_->OnRequestGetResponse(request, kNullResponse);
    waiting_list_.pop_front();
  }

  auto guard = shared_from_this();
  delegate_->OnClientChannelClosed(guard);
}

}//net
