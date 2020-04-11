#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "codec_message.h"

#include <net_io/url_utils.h>
#include <net_io/net_callback.h>
#include <net_io/tcp_channel.h>

namespace base {
  class MessageLoop;
}

namespace lt {
namespace net {

/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class CodecService : public SocketChannel::Reciever,
                     public EnableShared(CodecService) {
public:
  class Delegate {
    public:
      virtual void OnProtocolServiceReady(const RefCodecService& service) {};
      virtual void OnProtocolServiceGone(const RefCodecService& service) = 0;
      virtual void OnCodecMessage(const RefCodecMessage& message) = 0;
      //for client side
      virtual const url::RemoteInfo* GetRemoteInfo() const {return NULL;};
  };

public:
  CodecService(base::MessageLoop* loop);
  virtual ~CodecService() {};

  void SetDelegate(Delegate* d);
  /* this can be override for create diffrent type SocketChannel,
   * eg SSLChannel, UdpChannel, ....... */
  virtual bool BindToSocket(int fd,
                            const SocketAddr& local,
                            const SocketAddr& peer);

  virtual void StartProtocolService();

  TcpChannel* Channel() {return channel_.get();};
  base::MessageLoop* IOLoop() const { return binded_loop_;}

  void CloseService();
  bool IsConnected() const;

  virtual void BeforeCloseService() {};
  virtual void AfterChannelClosed() {};

  /* feature indentify*/
  //async clients request
  virtual bool KeepSequence() {return true;};
  virtual bool KeepHeartBeat() {return false;}

  virtual bool EncodeToChannel(CodecMessage* message) = 0;
  virtual bool EncodeResponseToChannel(const CodecMessage* req, CodecMessage* res) = 0;

  virtual const RefCodecMessage NewHeartbeat() {return NULL;}
  virtual const RefCodecMessage NewResponse(const CodecMessage*) {return NULL;}

  virtual const std::string& protocol() const {
    const static std::string kEmpty;
    return kEmpty;
  };

  void SetIsServerSide(bool server_side);
  bool IsServerSide() const {return server_side_;}
  inline MessageType InComingType() const {
    return server_side_ ? MessageType::kRequest : MessageType::kResponse;
  }
protected:
  // override this do initializing for client side, like set db, auth etc
  virtual void OnChannelReady(const SocketChannel*) override;

  void OnChannelClosed(const SocketChannel*) override;

  bool server_side_;
  RefTcpChannel channel_;
  Delegate* delegate_ = nullptr;
  base::MessageLoop* binded_loop_ = nullptr;
};

}}
#endif
