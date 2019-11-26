#include <set>
#include <functional>
#include <glog/logging.h>

#include "../tcp_channel.h"
#include "../socket_utils.h"
#include "../socket_acceptor.h"

using namespace lt;

base::MessageLoop loop;
std::set<net::RefTcpChannel> connections;

class EchoConsumer : public net::SocketChannel::Reciever {
public:
  void OnChannelClosed(const net::SocketChannel* ch) override {
    auto iter = connections.begin();
    for (; iter != connections.end(); iter++) {
      if (iter->get() == ch) {
        iter = connections.erase(iter);
        break;
      }
    }
    LOG(INFO) << "channel:[" << ch->ChannelName() << "] closed, connections count:" << connections.size();
  };

  void OnDataReceived(const net::SocketChannel* ch, net::IOBuffer *buf) override {
    auto iter = std::find_if(connections.begin(),
                             connections.end(),
                             [&](const net::RefTcpChannel& channel) -> bool {
                               return ch == channel.get();
                             });
    net::TcpChannel* channel = iter->get();
    channel->Send(buf->GetRead(), buf->CanReadSize());
    buf->Consume(buf->CanReadSize());
  };
};

EchoConsumer global_consumer;

int main(int argc, char** argv) {

  loop.Start();

  auto on_new_connection = [&](int fd, const net::SocketAddr& peer) {
    net::SocketAddr local(net::socketutils::GetLocalAddrIn(fd));
    auto ch = net::TcpChannel::Create(fd, local, peer, &loop);
    ch->SetReciever(&global_consumer);
    ch->Start();

    connections.insert(ch);
    LOG(INFO) << "channel:[" << ch->ChannelName() << "] connected, connections count:" << connections.size();
  };

  net::SocketAcceptor* acceptor;
  loop.PostTask(NewClosure([&]() {
    net::SocketAddr addr(5005);
    acceptor = new net::SocketAcceptor(loop.Pump(), addr);
    acceptor->SetNewConnectionCallback(std::bind(on_new_connection, std::placeholders::_1, std::placeholders::_2));
    //CHECK(acceptor->StartListen());
  }));

  loop.WaitLoopEnd();
  return 0;
}
