#include "../service_acceptor.h"

#include "../tcp_channel.h"
#include <glog/logging.h>
#include "../socket_utils.h"
#include <functional>

int main(int argc, char** argv) {

  base::MessageLoop2 loop;
  LOG(INFO) << "Call Start";
  loop.Start();
  LOG(INFO) << "Started";

  std::vector<net::RefTcpChannel> connections;

  auto new_connection = [&](int fd, const net::InetAddress& peer) {
    LOG(INFO) << " New Connection from:" << peer.IpPortAsString();

    net::InetAddress local(net::socketutils::GetLocalAddrIn(fd));
    auto f = net::TcpChannel::Create(fd,
                                     local,
                                     peer,
                                     &loop,
                                     net::ChannelServeType::kServerType);
    connections.push_back(f);

    int32_t size = f->Send((const uint8_t*)"const", sizeof "const");
    LOG(INFO) << "N ByteWrite:" << size;
  };

  net::ServiceAcceptor* acceptor;
  loop.PostTask(base::NewClosure([&]() {

    net::InetAddress addr(5005);
    acceptor = new net::ServiceAcceptor(loop.Pump(), addr);

    acceptor->SetNewConnectionCallback(std::bind(new_connection, std::placeholders::_1, std::placeholders::_2));
    acceptor->StartListen();
  }));


  loop.WaitLoopEnd();
  return 0;
}
