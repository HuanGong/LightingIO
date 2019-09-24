#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "channel.h"
#include "net_callback.h"
#include "tcp_channel.h"

#include "proto_message.h"

namespace lt {
namespace net {

class ProtoServiceDelegate {
public:
  virtual void OnProtocolServiceReady(const RefProtoService& service) {};
  virtual void OnProtocolServiceGone(const RefProtoService& service) = 0;
  virtual void OnProtocolMessage(const RefProtocolMessage& message) = 0;
};

/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService : public SocketChannel::Reciever,
                     public std::enable_shared_from_this<ProtoService> {
public:
  ProtoService();
  virtual ~ProtoService();

  void SetDelegate(ProtoServiceDelegate* d);
  /* this can be override if some protocol need difference channel, eg SSLChannel, UdpChannel */
  virtual bool BindToSocket(int fd,
                            const SocketAddr& local,
                            const SocketAddr& peer,
                            base::MessageLoop* loop);

  TcpChannel* Channel() {return channel_.get();};
  base::MessageLoop* IOLoop() {return channel_ ? channel_->IOLoop() : NULL;}

  void CloseService();
  bool IsConnected() const;

  virtual void BeforeCloseService() {};
  virtual void AfterChannelClosed() {};
  virtual void StartHeartBeat(int32_t ms) {};

  //async clients request
  virtual bool KeepSequence() {return true;};


  virtual bool SendRequestMessage(const RefProtocolMessage& message) = 0;
  virtual bool SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) = 0;

  virtual const RefProtocolMessage NewResponseFromRequest(const RefProtocolMessage &) {return NULL;}

  void SetIsServerSide(bool server_side);
  bool IsServerSide() const {return server_side_;}
  inline MessageType InComingType() const {
    return server_side_ ? MessageType::kRequest : MessageType::kResponse;
  }
protected:
  void OnChannelReady(const SocketChannel*) override;
  void OnChannelClosed(const SocketChannel*) override;

  RefTcpChannel channel_;
  bool server_side_;
  ProtoServiceDelegate* delegate_ = NULL;
};

}}
#endif
