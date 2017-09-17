#include "cstdlib"
#include "unistd.h"
#include "ctime"
#include <time.h>

#include "http_server.h"

int main() {

  // use default delegate
  net::HttpServer server(NULL);
  server.Initialize();

  server.Run();

  while(1) {
    sleep(10);
  }
  return 0;
}
