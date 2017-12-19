#ifndef _NET_IO_SERVICE_H_H
#define _NET_IO_SERVICE_H_H

#include "net_callback.h"
#include "service_acceptor.h"
#include "base/event_loop/msg_event_loop.h"
namespace net {

class IOServiceDelegate {
public:
  virtual ~IOServiceDelegate();
  virtual base::MessageLoop2* GetAcceptorLoop() = 0;
  virtual base::MessageLoop2* NextIOHandleLoop() = 0;
};

class IOService {
public:
  /* Must Construct in ownerloop, why? bz we want all io level is clear and tiny
   * it only handle io relative things, it's easy! just post a task IOMain at everything
   * begin */
  IOService(const InetAddress local,
            IOServiceDelegate* delegate);

  ~IOService();

  void StartIOService();
  void StopIOService();

private:
  /* create a new connection channel */
  void HandleNewConnection(int peer_fd, const InetAddress& peer_addr);

  RefServiceAcceptor acceptor_;
  base::MessageLoop2* io_loop_;
};

}
#endif
