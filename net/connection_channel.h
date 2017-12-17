#ifndef NET_CHANNAL_CONNECTION_H_
#define NET_CHANNAL_CONNECTION_H_

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

class ConnectionChannel : public std::enable_shared_from_this<ConnectionChannel> {
public:
  typedef enum {
    CONNECTED = 0,
    CONNECTING = 1,
    DISCONNECTED = 2,
    DISCONNECTING = 3
  } ConnStatus;

  static RefConnectionChannel Create(int socket_fd,
                                     const InetAddress& local,
                                     const InetAddress& peer,
                                     base::MessageLoop2* loop);

  ~ConnectionChannel();

  void Initialize();

  void SetChannalName(const std::string name);

  void SetDataHandleCallback(const DataRcvCallback& callback) {
    recv_data_callback_ = callback;
  }
  void SetFinishSendCallback(const DataWritenCallback& callback) {
    finish_write_callback_ = callback;
  }
  void SetConnectionCloseCallback(const ConnectionClosedCallback& callback) {
    closed_callback_ = callback;
  }
  void SetStatusChangedCallback(const ConnectionStatusCallback& callback) {
    status_change_callback_ = callback;
  }

  inline bool ConnectionStatus() {return channel_status_; }
  std::string StatusToString() const;
  /*return -1 in error, return 0 when success*/
  int32_t Send(const char* data, const int32_t len);
protected:
  void HandleRead();
  void HandleWrite();
  void HandleError();
  void HandleClose();

  void OnStatusChanged();
private:
  ConnectionChannel(int fd,
                const InetAddress& loc,
                const InetAddress& peer,
                base::MessageLoop2* loop);

private:
  base::MessageLoop2* owner_loop_;

  ConnStatus channel_status_;

  int socket_fd_;
  base::RefFdEvent fd_event_;

  InetAddress local_addr_;
  InetAddress peer_addr_;

  std::string channal_name_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  DataRcvCallback recv_data_callback_;
  ConnectionClosedCallback closed_callback_;
  DataWritenCallback finish_write_callback_;
  ConnectionStatusCallback status_change_callback_;
  DISALLOW_COPY_AND_ASSIGN(ConnectionChannel);
};

}
#endif
