#ifndef NET_TCP_CHANNAL_CONNECTION_H_
#define NET_TCP_CHANNAL_CONNECTION_H_

#include <memory>
#include <functional>

#include "net_callback.h"
#include "io_buffer.h"
#include "inet_address.h"
#include "base/base_micro.h"
#include "base/message_loop/message_loop.h"
/* *
 *
 * all of this thing should happend in own loop, include callbacks
 *
 * */
namespace net {

typedef enum {
  kClientType = 0,
  kServerType = 1
} ChannelServeType;

class TcpChannel : public std::enable_shared_from_this<TcpChannel> {
public:
  typedef enum {
    CONNECTING = 0,
    CONNECTED = 1,
    DISCONNECTED = 2,
    DISCONNECTING = 3
  } ChannelStatus;

  static RefTcpChannel Create(int socket_fd,
                              const InetAddress& local,
                              const InetAddress& peer,
                              base::MessageLoop* loop,
                              ChannelServeType type);

  static RefTcpChannel CreateClientChannel(int socket_fd,
                                           const InetAddress& local,
                                           const InetAddress& remote,
                                           base::MessageLoop* loop);
  static RefTcpChannel CreateServerChannel(int socket_fd,
                                           const InetAddress& local,
                                           const InetAddress& peer,
                                           base::MessageLoop* loop);

  TcpChannel(int socket_fd,
             const InetAddress& loc,
             const InetAddress& peer,
             base::MessageLoop* loop,
             ChannelServeType type);
  ~TcpChannel();

  void Start();

  const std::string& ChannelName() {return channal_name_;}
  void SetChannelName(const std::string name);
  void SetOwnerLoop(base::MessageLoop* owner);
  void SetDataHandleCallback(const RcvDataCallback& callback);
  void SetFinishSendCallback(const FinishSendCallback& callback);
  void SetCloseCallback(const ChannelClosedCallback& callback);
  void SetStatusChangedCallback(const ChannelStatusCallback& callback);
  void SetProtoService(RefProtoService proto_service);

  /* send a protocol message*/
  bool SendProtoMessage(RefProtocolMessage message);
  /* return -1 in error, return 0 when success*/
  int32_t Send(const uint8_t* data, const int32_t len);

  void ForceShutdown();
  void ShutdownChannel();
  const std::string StatusAsString();
  bool InIOLoop() const;
  bool IsConnected() const;
  base::MessageLoop* IOLoop() const;
  bool IsServerChannel() const;
  bool IsClientChannel() const;
  RefProtoService GetProtoService() const;
protected:
  void HandleRead();
  void HandleWrite();
  void HandleError();
  void HandleClose();

  void SetChannelStatus(ChannelStatus st);
private:
  void OnConnectionReady();
private:
  /* all the io thing happend in work loop,
   * and the life cycle managered by ownerloop
   * */
  base::MessageLoop* work_loop_;
  base::MessageLoop* owner_loop_;

  bool schedule_shutdown_;
  ChannelStatus channel_status_;

  base::RefFdEvent fd_event_;

  InetAddress local_addr_;
  InetAddress peer_addr_;

  const ChannelServeType serve_type_;

  std::string channal_name_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  RefProtoService proto_service_;

  RcvDataCallback recv_data_callback_;
  ChannelClosedCallback closed_callback_;
  FinishSendCallback finish_write_callback_;
  ChannelStatusCallback status_change_callback_;
  DISALLOW_COPY_AND_ASSIGN(TcpChannel);
};

}
#endif
