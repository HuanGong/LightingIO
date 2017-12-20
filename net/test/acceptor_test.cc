#include "../service_acceptor.h"

#include "../connection_channel.h"
#include <glog/logging.h>
#include "../socket_utils.h"
#include <functional>

int main(int argc, char** argv) {

  int count = 0;

  base::MessageLoop2 loop;
  LOG(INFO) << "Call Start";
  loop.Start();
  LOG(INFO) << "Started";

  std::vector<net::RefConnectionChannel> connections;

  auto new_connection = [&](int fd, const net::InetAddress& peer) {
    LOG(INFO) << " New Connection from:" << peer.IpPortAsString();

    net::InetAddress local(net::socketutils::GetLocalAddrIn(fd));
    auto f = net::ConnectionChannel::Create(fd,
                                            local,
                                            peer,
                                            &loop);
    connections.push_back(f);

    int32_t size = f->Send("const", sizeof "const");
    LOG(INFO) << "N ByteWrite:" << size;
  };

  net::ServiceAcceptor* acceptor;
  loop.PostTask(base::NewClosure([&]() {

    net::InetAddress addr(5005);
    acceptor = new net::ServiceAcceptor(&loop, addr);

    acceptor->SetNewConnectionCallback(std::bind(new_connection, std::placeholders::_1, std::placeholders::_2));
    acceptor->StartListen();
  }));


  loop.WaitLoopEnd();
  return 0;
}
