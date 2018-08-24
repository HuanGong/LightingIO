#ifndef _LT_NET_CLIENT_CHANNEL_H
#define _LT_NET_CLIENT_CHANNEL_H

#include "tcp_channel.h"
#include "net_callback.h"
#include "requests_keeper.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service.h"

namespace net {

class ClientChannel;

typedef std::shared_ptr<ClientChannel> RefClientChannel;
class ClientChannel {
public:
  class Delegate {
  public:
    virtual void OnClientChannelClosed(const RefClientChannel& channel) = 0;
    virtual void OnRequestGetResponse(const RefProtocolMessage&, const RefProtocolMessage&) = 0;
  };
  ClientChannel(Delegate* d, RefTcpChannel& ch)
    : delegate_(d),
      channel_(ch) {
  }
  virtual ~ClientChannel() {}
  virtual void StartClient() = 0;
  void CloseClientChannel() {
    if (channel_->InIOLoop()) {
      channel_->ShutdownChannel();
      return;
    }
    auto functor = std::bind(&TcpChannel::ShutdownChannel, channel_);
    IOLoop()->PostTask(base::NewClosure(functor));
  }
  virtual void SendRequest(RefProtocolMessage request) = 0;

  base::MessageLoop* IOLoop() {return channel_->IOLoop();};
  void SetRequestTimeout(uint32_t ms) {request_timeout_ = ms;};
protected:
  Delegate* delegate_;
  RefTcpChannel channel_;
  uint32_t request_timeout_ = 1000; //1s
};



}//end namespace
#endif
