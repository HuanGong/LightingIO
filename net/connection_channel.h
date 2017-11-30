#ifndef NET_CHANNAL_CONNECTION_H_
#define NET_CHANNAL_CONNECTION_H_

#include <memory>
#include <functional>

#include "inet_address.h"
#include "base/base_micro.h"
#include "base/event_loop/msg_event_loop.h"

namespace net {

class SocketConnection : public std::enable_shared_from_this<SocketConnection> {
public:
  std::shared_ptr<SocketConnection> Create(int socket_fd,
                                        const InetAddress& local,
                                        const InetAddress& peer,
                                        base::Messageloop2* loop);
  void SetChannalName(const std::string name);
private:
  SocketConnection(int fd,
                const InetAddress& loc,
                const InetAddress& peer,
                base::MessageLoop2* loop);
  ~SocketConnection();

protected:
  void HandleRead();
  void HandleWrite();
  void HandleError();
  void HandleClose();

private:
  base::MessageLoop2* owner_loop_;

  int socket_fd_;
  base::RefFdEvent fd_event_;

  InetAddress local_addr_;
  InetAddress peer_addr_;

  std::string channal_name_;
  DISALLOW_COPY_AND_ASSIGN(ConnectionChannal);
};

}
#endif
