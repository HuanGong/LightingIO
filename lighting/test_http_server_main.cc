#include "cstdlib"
#include "unistd.h"
#include "ctime"
#include <time.h>

#include "http_server.h"

int main() {

  net::Server server(NULL);

  std::vector<std::string> service = {std::string("0.0.0.0:6666")};

  server.InitWithAddrPorts(service);

  server.Run();

  while(1) {
    sleep(10);
  }
  return 0;
}
