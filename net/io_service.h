#ifndef _NET_IO_SERVICE_H_H
#define _NET_IO_SERVICE_H_H

#include <atomic>

#include "net_callback.h"
#include "service_acceptor.h"
#include "base/event_loop/msg_event_loop.h"

namespace net {

class IOServiceDelegate {
public:
  virtual ~IOServiceDelegate(){};
  virtual base::MessageLoop2* GetNextIOWorkLoop() = 0;

  /* use for couting connection numbers and limit max connections
   * return false when reach max connections, otherwise return true*/
  virtual bool IncreaseChannelCount() = 0;
  virtual void DecreaseChannelCount() = 0;

  virtual bool CanCreateNewChannel() { return true; }
  virtual RefProtoService GetProtocolService(const std::string protocol) = 0;
};

class IOService {
public:
  /* Must Construct in ownerloop, why? bz we want all io level is clear and tiny
   * it only handle io relative things, it's easy! just post a task IOMain at everything
   * begin */
  IOService(const InetAddress local,
            const std::string protocol,
            base::MessageLoop2* workloop,
            IOServiceDelegate* delegate);

  ~IOService();

  void StartIOService();
  void StopIOService();

private:
  /* create a new connection channel */
  void HandleNewConnection(int, const InetAddress&);

  void OnChannelClosed(const RefTcpChannel&);

  /* store[remove] connection to[from] maps, only run on work_loop */
  void StoreConnection(const RefTcpChannel connetion);
  void RemoveConncetion(const RefTcpChannel connection);

  bool as_dispatcher_;
  std::string protocol_;
  RefServiceAcceptor acceptor_;

  base::MessageLoop2* work_loop_;

  /* interface to owner and handler */
  IOServiceDelegate* delegate_;
  RefProtoService proto_service_;

  std::atomic<int64_t> channel_count_;
  std::map<std::string, RefTcpChannel> connections_;
};

}
#endif
