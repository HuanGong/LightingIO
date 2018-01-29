#ifndef _NET_CLIENTS_MANAGER_H_H
#define _NET_CLIENTS_MANAGER_H_H

#include "client_router.h"

namespace net {


class ClientsManager {
public:
  ClientsManager();
  ~ClientsManager();

  void RegisterClient();
private:

};

} //end net
#endif
