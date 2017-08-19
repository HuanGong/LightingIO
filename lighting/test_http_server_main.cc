#include "cstdlib"
#include "unistd.h"
#include "ctime"
#include <time.h>

#include "http_server.h"

int main() {
  net::SrvConfig config = {false, {8009}, 2};
  net::HttpSrv server(NULL, config);

  while(1) {
    sleep(10);
  }
  return 0;
}
