#ifndef SERVER_CONFIG_H_H
#define SERVER_CONFIG_H_H

#include <string>
#include <unistd.h>

namespace net {

class ServerConf {
public:
  virtual ~ServerConf();

  const std::string ListenAddressPorts();
  int WorkerThreadCount();
};

}
#endif
