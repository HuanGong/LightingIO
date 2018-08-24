#ifndef _LT_NET_ASYNC_CLIENT_CHANNEL_H
#define _LT_NET_ASYNC_CLIENT_CHANNEL_H

#include <list>
#include "tcp_channel.h"
#include "net_callback.h"
#include "queued_channel.h"

#include <unordered_map>
#include "protocol/proto_message.h"
#include "protocol/proto_service.h"

namespace net {

class AsyncChannel;

typedef std::shared_ptr<AsyncChannel> RefAsyncChannel;

class AsyncChannel : public ClientChannel,
                     public std::enable_shared_from_this<AsyncChannel> {
public:
  AsyncChannel(Delegate*, RefTcpChannel&);
  ~AsyncChannel();

  void StartClient() override;
  void SendRequest(RefProtocolMessage request) override;
private:
  bool TrySendRequestInternal();
  void OnRequestTimeout(WeakProtocolMessage request);
  void OnChannelClosed(const RefTcpChannel& channel);
  void OnResponseMessage(const RefProtocolMessage& res);
private:
  std::unordered_map<uint64_t, RefProtocolMessage> in_progress_;
};

}//end namespace
#endif
