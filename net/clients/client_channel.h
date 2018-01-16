#ifndef _LT_NET_CLIENT_CHANNEL_H
#define _LT_NET_CLIENT_CHANNEL_H

#include "tcp_channel.h"
#include "net_callback.h"

#include "protocol/proto_message.h"
#include "protocol/proto_service.h"

namespace net {
class ClientChannel;

typedef std::shared_ptr<ClientChannel> RefClientChannel;
typedef std::function<void (RefProtocolMessage&, RefProtocolMessage&)> ResponseHandler;

/* all thing doing in iocontext */
class ClientChannel : public std::enable_shared_from_this<ClientChannel> {
public:
  RefClientChannel Create(RefTcpChannel& channel);

  ClientChannel(RefTcpChannel&);
  virtual ~ClientChannel();

  bool StartRequest(RefProtocolMessage& request);
  void OnResponseMessage(RefProtocolMessage message);
  void SetResponseHandler(ResponseHandler handler);
private:
  void OnChannelClosed(RefTcpChannel channel);

  //sequence_keeper_;
  RefTcpChannel channel_;

  ResponseHandler response_handler_;
};

}
#endif
