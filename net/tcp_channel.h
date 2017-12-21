#ifndef NET_TCP_CHANNAL_CONNECTION_H_
#define NET_TCP_CHANNAL_CONNECTION_H_

#include <memory>
#include <functional>

#include "net_callback.h"
#include "io_buffer.h"
#include "inet_address.h"
#include "base/base_micro.h"
#include "base/event_loop/msg_event_loop.h"
/* *
 *
 * all of this thing should happend in own loop, include callbacks
 *
 * */
namespace net {

class TcpChannel : public std::enable_shared_from_this<TcpChannel> {
public:
  typedef enum {
    CONNECTED = 0,
    CONNECTING = 1,
    DISCONNECTED = 2,
    DISCONNECTING = 3
  } ChannelStatus;

  static RefTcpChannel Create(int socket_fd,
                                     const InetAddress& local,
                                     const InetAddress& peer,
                                     base::MessageLoop2* loop);

  ~TcpChannel();
  const std::string& ChannelName() {return channal_name_;}
  void SetChannelName(const std::string name);
  void SetOwnerLoop(base::MessageLoop2* owner);
  void SetDataHandleCallback(const RcvDataCallback& callback);
  void SetFinishSendCallback(const FinishSendCallback& callback);
  void SetCloseCallback(const ChannelClosedCallback& callback);
  void SetStatusChangedCallback(const ChannelStatusCallback& callback);

  std::string StatusToString() const;
  /*return -1 in error, return 0 when success*/
  int32_t Send(const char* data, const int32_t len);

  void ShutdownConnection();

  inline ChannelStatus ConnectionStatus() {return channel_status_; }
protected:
  void Initialize();

  void HandleRead();
  void HandleWrite();
  void HandleError();
  void HandleClose();

  void OnStatusChanged();
private:
  TcpChannel(int socket_fd,
             const InetAddress& loc,
             const InetAddress& peer,
             base::MessageLoop2* loop);
  void OnConnectionReady();
private:
  /* all the io thing happend in work loop,
   * and the life cycle managered by ownerloop
   * */
  base::MessageLoop2* work_loop_;
  base::MessageLoop2* owner_loop_;

  ChannelStatus channel_status_;

  int socket_fd_;
  base::RefFdEvent fd_event_;

  InetAddress local_addr_;
  InetAddress peer_addr_;

  std::string channal_name_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  RcvDataCallback recv_data_callback_;
  ChannelClosedCallback closed_callback_;
  FinishSendCallback finish_write_callback_;
  ChannelStatusCallback status_change_callback_;
  DISALLOW_COPY_AND_ASSIGN(TcpChannel);
};

}
#endif
