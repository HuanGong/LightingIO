
#include "client_channel.h"

#include "requests_keeper.h"
#include "base/base_constants.h"

namespace net {

const static RefProtocolMessage kNullResponse;

RefClientChannel ClientChannel::Create(Delegate* delegate, RefTcpChannel& channel) {
  return std::make_shared<ClientChannel>(delegate, channel);
}

ClientChannel::ClientChannel(Delegate* delegate, RefTcpChannel& channel)
  : delegate_(delegate),
    message_timeout_(10000),
    channel_(channel) {

  CHECK(delegate_);
  requests_keeper_.reset(new RequestsKeeper());

  channel_->SetCloseCallback(std::bind(&ClientChannel::OnChannelClosed,
                                       this, std::placeholders::_1));
}

ClientChannel::~ClientChannel() {
}

bool ClientChannel::ScheduleARequest(RefProtocolMessage request) {
  CHECK(channel_->InIOLoop());

  requests_keeper_->PendingRequest(request);

  if (requests_keeper_->InProgressRequest()) {
    return true;
  }

  TryFireNextRequest();

  return true;
}

void ClientChannel::OnChannelClosed(RefTcpChannel channel) {
  VLOG(GLOG_VTRACE) << "Client's TcpChannel Closed";
  CHECK(channel_->InIOLoop());

  RefProtocolMessage request = requests_keeper_->InProgressRequest();
  if (request) {
    OnRequestFailed(request, FailInfo::kChannelBroken);
    requests_keeper_->ResetCurrent();
  }

  //resume all requests tell them request failed
  while(requests_keeper_->InQueneCount()) {
    RefProtocolMessage next_request = requests_keeper_->PopNextRequest();
    if (!next_request) {
      continue;
    }
    OnRequestFailed(next_request, FailInfo::kChannelBroken);
  };

  VLOG(GLOG_VTRACE) << " OnClientChannelClosed, Channel:" << channel_->ChannelName();
  delegate_->OnClientChannelClosed(shared_from_this());
}

void ClientChannel::OnResponseMessage(RefProtocolMessage response) {
  RefProtocolMessage request = requests_keeper_->InProgressRequest();
  if (!request) {
    LOG(ERROR) << "Got A Response Without Request or Request Has Canceled";
    return ;
  }

  CHECK(channel_->InIOLoop());

  OnBackResponse(request, response);
  requests_keeper_->ResetCurrent();

  TryFireNextRequest();
}

bool ClientChannel::TryFireNextRequest() {
  CHECK(channel_->InIOLoop());

  if (requests_keeper_->InProgressRequest()) {
    return false;
  }

  LOG(ERROR) << " Try Send A Request by tcp Channel";

  bool success = false;
  do {

    RefProtocolMessage next_request = requests_keeper_->PopNextRequest();
    if (!next_request) {
      return false;
    }

    bool success = channel_->SendProtoMessage(next_request);

    if (success) {
      requests_keeper_->SetCurrent(next_request);

      auto functor = std::bind(&ClientChannel::OnRequestTimeout, shared_from_this(), next_request);
      IOLoop()->PostDelayTask(base::NewClosure(functor), message_timeout_);

    } else {
      //Notify Woker[Sender] For Failed Request Sending
      OnRequestFailed(next_request, FailInfo::kChannelBroken);
    }
  } while(!success && requests_keeper_->InQueneCount());

  return success;
}

void ClientChannel::OnRequestTimeout(RefProtocolMessage request) {
  CHECK(channel_->InIOLoop());

  if (request != requests_keeper_->InProgressRequest()) {
    LOG(INFO) << " This Request Has Got Response, Ignore this Timeout";
    return;
  }

  if (channel_->IsConnected()) {
    channel_->ForceShutdown();
  } else {
    OnChannelClosed(channel_);
  }
}

void ClientChannel::OnRequestFailed(RefProtocolMessage& request, FailInfo reason) {
  //on weak up for failed reason
  request->SetFailInfo(reason);
  delegate_->OnRequestGetResponse(request, kNullResponse);
}

void ClientChannel::OnBackResponse(RefProtocolMessage& request, RefProtocolMessage& response) {
  delegate_->OnRequestGetResponse(request, response);
}

void ClientChannel::CloseClientChannel() {
  if (channel_->InIOLoop()) {
    channel_->ForceShutdown();
    return;
  }
  IOLoop()->PostTask(base::NewClosure(std::bind(&TcpChannel::ForceShutdown, channel_)));
}

void ClientChannel::OnSendFinished(RefTcpChannel channel) {
  LOG(INFO) << " ClientChannel::OnSendFinished";
}

void ClientChannel::SetRequestTimeout(uint32_t ms) {
  message_timeout_ = ms; //ms
}

base::MessageLoop2* ClientChannel::IOLoop() {
  return channel_->IOLoop();
}

}//end net
