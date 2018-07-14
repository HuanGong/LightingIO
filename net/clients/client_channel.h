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

typedef std::unique_ptr<RequestsKeeper> OwnedRequestsKeeper;
typedef std::function<void (RefProtocolMessage&, RefProtocolMessage&)> ResponseHandler;

/* all thing doing in iocontext */
class ClientChannel : public std::enable_shared_from_this<ClientChannel> {
public:
  class Delegate {
  public:
    virtual void OnClientChannelClosed(RefClientChannel channel) = 0;
    virtual void OnRequestGetResponse(RefProtocolMessage, RefProtocolMessage) = 0;
  };

  RefClientChannel Create(Delegate* delegate, RefTcpChannel& channel);

  ClientChannel(Delegate*, RefTcpChannel&);
  virtual ~ClientChannel();

  base::MessageLoop* IOLoop();
  void SetRequestTimeout(uint32_t ms);
  bool ScheduleARequest(RefProtocolMessage request);
  void OnResponseMessage(RefProtocolMessage message);
  void CloseClientChannel();
private:
  bool TryFireNextRequest();

  void OnRequestFailed(RefProtocolMessage& request, FailInfo reason);
  void OnBackResponse(RefProtocolMessage& request, RefProtocolMessage& response);

  bool SendRequest(RefProtocolMessage& request);
  void OnChannelClosed(RefTcpChannel channel);
  void OnSendFinished(RefTcpChannel channel);
  void OnRequestTimeout(RefProtocolMessage request);

private:
  Delegate* delegate_;
  uint32_t message_timeout_; //ms

  RefTcpChannel channel_;
  OwnedRequestsKeeper requests_keeper_;
};

}//end namespace
#endif
