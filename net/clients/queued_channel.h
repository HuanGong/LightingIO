#ifndef _LT_NET_QUEUED_CLIENT_CHANNEL_H 
#define _LT_NET_QUEUED_CLIENT_CHANNEL_H

#include <list>
#include "tcp_channel.h"
#include "net_callback.h"
#include "client_channel.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service.h"

namespace net {

class QueuedChannel;
typedef std::shared_ptr<QueuedChannel> RefQueuedChannel;

class QueuedChannel : public ClientChannel,
                      public std::enable_shared_from_this<QueuedChannel> {
public:
  QueuedChannel(Delegate*, RefTcpChannel&);
  ~QueuedChannel();

  void StartClient() override;
  void SendRequest(RefProtocolMessage request) override;
private:
  bool TrySendRequestInternal();
  void OnRequestTimeout(WeakProtocolMessage request);
  void OnChannelClosed(const RefTcpChannel& channel);
  void OnResponseMessage(const RefProtocolMessage& res);
private:
  RefProtocolMessage in_progress_request_;
  std::list<RefProtocolMessage> waiting_list_;
};

RefClientChannel CreateClientChannel(ClientChannel::Delegate* delegate, RefTcpChannel& channel);

}//end namespace
#endif
