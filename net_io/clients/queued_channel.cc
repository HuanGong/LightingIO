#include "async_channel.h"
#include "queued_channel.h"
#include "base/base_constants.h"
#include "protocol/proto_service.h"

namespace net {

RefQueuedChannel QueuedChannel::Create(Delegate* d, RefProtoService& s) {
  return RefQueuedChannel(new QueuedChannel(d, s));
}

QueuedChannel::QueuedChannel(Delegate* d, RefProtoService& service)
  : ClientChannel(d, service) {
}

QueuedChannel::~QueuedChannel() {
}

void QueuedChannel::StartClient() {
  //common part
  ClientChannel::StartClient();

  ProtoMessageHandler res_handler =
      std::bind(&QueuedChannel::OnResponseMessage,this, std::placeholders::_1);

  protocol_service_->SetMessageHandler(std::move(res_handler));
}

void QueuedChannel::SendRequest(RefProtocolMessage request)  {
  CHECK(IOLoop()->IsInLoopThread());

  request->SetIOContext(protocol_service_);
  waiting_list_.push_back(std::move(request));

  TrySendNext();
}

bool QueuedChannel::TrySendNext() {
  if (ing_request_) {
    return true;
  }

  bool success = false;
  while(!success && waiting_list_.size()) {

    RefProtocolMessage& next = waiting_list_.front();
    success = protocol_service_->SendRequestMessage(next);
    waiting_list_.pop_front();
    if (success) {
      ing_request_ = next;
    } else {
      next->SetFailCode(MessageCode::kConnBroken);
      delegate_->OnRequestGetResponse(next, ProtocolMessage::kNullMessage);
    }
  }

  if (success) {
    DCHECK(ing_request_);
    WeakProtocolMessage weak(ing_request_);   // weak ptr must init outside, Take Care of weakptr
    auto functor = std::bind(&QueuedChannel::OnRequestTimeout, shared_from_this(), weak);
    IOLoop()->PostDelayTask(NewClosure(functor), request_timeout_);
  }
  return success;
}

void QueuedChannel::OnRequestTimeout(WeakProtocolMessage weak) {

  RefProtocolMessage request = weak.lock();
  if (!request) return;

  if (request.get() != ing_request_.get()) {
    return;
  }

  VLOG(GLOG_VINFO) << __FUNCTION__ << protocol_service_->Channel()->ChannelInfo() << " timeout reached";

  request->SetFailCode(MessageCode::kTimeOut);
  delegate_->OnRequestGetResponse(request, ProtocolMessage::kNullMessage);
  ing_request_.reset();

  if (protocol_service_->IsConnected()) {
  	protocol_service_->CloseService();
  } else {
  	OnProtocolServiceGone(protocol_service_);
  }
}

void QueuedChannel::OnResponseMessage(const RefProtocolMessage& res) {
  DCHECK(IOLoop()->IsInLoopThread());

  if (!ing_request_) {
    LOG(ERROR) << __FUNCTION__ << " got response without request or request has canceled";
    return ;
  }

  delegate_->OnRequestGetResponse(ing_request_, res);
  ing_request_.reset();

  TrySendNext();
}

void QueuedChannel::OnProtocolServiceGone(const RefProtoService& service) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << service->Channel()->ChannelInfo() << " protocol service closed";

  if (ing_request_) {
    ing_request_->SetFailCode(MessageCode::kConnBroken);
    delegate_->OnRequestGetResponse(ing_request_, ProtocolMessage::kNullMessage);
    ing_request_.reset();
  }

  while(waiting_list_.size()) {
    RefProtocolMessage& request = waiting_list_.front();
    request->SetFailCode(MessageCode::kConnBroken);
    delegate_->OnRequestGetResponse(request, ProtocolMessage::kNullMessage);
    waiting_list_.pop_front();
  }

  auto guard = shared_from_this();
  delegate_->OnClientChannelClosed(guard);
}

}//net
