#include "../service_acceptor.h"


int main(int argc, char** argv) {

  int count = 0;

  base::MessageLoop2 loop;
  loop.Start();
  sleep(1);

  net::ServiceAcceptor* acceptor;
  loop.PostTask(base::NewClosure([&]() {

    net::InetAddress addr(5005);
    acceptor = new net::ServiceAcceptor(&loop, addr);
    acceptor->StartListen();
  }));


  loop.WaitLoopEnd();
  return 0;
}
