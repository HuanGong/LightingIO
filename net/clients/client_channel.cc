
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
    message_timeout_(0),
    channel_(channel) {

  CHECK(delegate_);
  requests_keeper_.reset(new RequestsKeeper());

  channel_->SetCloseCallback(std::bind(&ClientChannel::OnChannelClosed,
                                       this, std::placeholders::_1));
}

ClientChannel::~ClientChannel() {
  VLOG(GLOG_VTRACE) << "ClientChannel " << channel_->ChannelName() << " Gone";
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
  CHECK(channel_->InIOLoop());
  VLOG(GLOG_VTRACE) << " OnClientChannelClosed, Channel:" << channel_->ChannelName();

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

  std::shared_ptr<ClientChannel> guard = shared_from_this();
  delegate_->OnClientChannelClosed(guard);
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

  bool success = false;
  do {
    RefProtocolMessage next_request = requests_keeper_->PopNextRequest();
    if (!next_request) {return false;}

    success = channel_->SendProtoMessage(next_request);

    if (success) {
      requests_keeper_->SetCurrent(next_request);
      if (message_timeout_) {
        LOG(INFO) << " Schedule A Timeout For Outing Request";
        auto functor = std::bind(&ClientChannel::OnRequestTimeout,
                                 shared_from_this(),
                                 next_request);
        IOLoop()->PostDelayTask(base::NewClosure(functor), message_timeout_);
      }
    } else {
      //Notify Woker[Sender] For Failed Request Sending
      OnRequestFailed(next_request, FailInfo::kChannelBroken);
    }
  } while(!success && requests_keeper_->InQueneCount());

  return success;
}

void ClientChannel::OnRequestTimeout(RefProtocolMessage request) {
  CHECK(channel_->InIOLoop());

  //TODO: impl a cancelable timerevent
  if (request != requests_keeper_->InProgressRequest()) {
    VLOG(GLOG_VTRACE) << " This Request Has Got Response, Ignore Timeout";
    return;
  }

  if (channel_->IsConnected()) {
    LOG(INFO) << " ShutdownChannel for Timeout";
    channel_->ShutdownChannel();
    //channel_->ForceShutdown();
  } else {
    OnChannelClosed(channel_);
  }
}

void ClientChannel::OnRequestFailed(RefProtocolMessage& request, FailInfo reason) {
  request->SetFailInfo(reason);
  delegate_->OnRequestGetResponse(request, kNullResponse);
}

void ClientChannel::OnBackResponse(RefProtocolMessage& request, RefProtocolMessage& response) {
  delegate_->OnRequestGetResponse(request, response);
}

void ClientChannel::CloseClientChannel() {
  if (channel_->InIOLoop()) {
    //channel_->ForceShutdown();
    channel_->ShutdownChannel();
    return;
  }
  auto functor = std::bind(&TcpChannel::ShutdownChannel, channel_);
  IOLoop()->PostTask(base::NewClosure(functor));
}

void ClientChannel::OnSendFinished(RefTcpChannel channel) {
}

void ClientChannel::SetRequestTimeout(uint32_t ms) {
  message_timeout_ = ms; //ms
}

base::MessageLoop* ClientChannel::IOLoop() {
  return channel_->IOLoop();
}

}//end net
