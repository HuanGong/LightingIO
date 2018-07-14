#ifndef _NET_SERVICE_ACCEPTOR_H_H_
#define _NET_SERVICE_ACCEPTOR_H_H_

#include "inet_address.h"
#include "socket_utils.h"

#include "net_callback.h"
#include "base/base_micro.h"
#include "base/message_loop/fd_event.h"
#include "base/message_loop/message_loop.h"

namespace net {

class ServiceAcceptor {
public:
  //TODO: consider using eventpump replace loop
  ServiceAcceptor(base::EventPump*, const InetAddress&);
  ~ServiceAcceptor();

  bool StartListen();
  void StopListen();
  bool IsListenning() { return listenning_; }

  void SetNewConnectionCallback(const NewConnectionCallback& cb);
  const InetAddress& ListenLocal() const { return address_; };
private:
  void InitListener();
  void HandleCommingConnection();

  bool listenning_;
  base::EventPump* event_pump_;

  int socket_fd_;
  base::FdEvent::RefFdEvent socket_event_;

  InetAddress address_;
  NewConnectionCallback new_conn_callback_;

private:
  DISALLOW_COPY_AND_ASSIGN(ServiceAcceptor);
};

} //end net
#endif
//end of file
