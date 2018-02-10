#include "../service_acceptor.h"

#include "../net_callback.h"
#include "../tcp_channel.h"
#include "../socket_utils.h"

#include <functional>
#include <glog/logging.h>
#include <net/io_buffer.h>
#include <net/protocol/http/http_request.h>
#include <net/protocol/http/http_response.h>
#include <net/protocol/http/http_proto_service.h>
#include <sstream>


int main(int argc, char** argv) {

  net::RefHttpRequest request =
    std::make_shared<net::HttpRequest>(net::IODirectionType::kOutRequest);
  request->SetMethod("GET");
  request->MutableBody() = "Nice to Meet you!";
  std::string field("Name");
  std::string value("HuanGong");
  request->InsertHeader(field, value);
  request->SetRequestURL("/abc");
  net::IOBuffer req_buffer;
  net::HttpProtoService::RequestToBuffer(request.get(), &req_buffer);
  LOG(ERROR) << "request:\n"  << req_buffer.AsString();


  net::RefHttpResponse response =
    std::make_shared<net::HttpResponse>(net::IODirectionType::kOutResponse);
  response->SetResponseCode(200);
  response->SetKeepAlive(true);
  response->MutableBody() = "Nice to meet your,I'm LightingIO";

  net::IOBuffer buffer;
  net::HttpProtoService::ResponseToBuffer(response.get(), &buffer);
  LOG(ERROR) << "response:\n"  << buffer.AsString();


  return 0;
#if 0
  int count = 0;

  base::MessageLoop2 loop;
  loop.SetLoopName("loop1");
  loop.Start();

  base::MessageLoop2 l2;
  l2.SetLoopName("loop2");
  l2.Start();

  base::MessageLoop2 l3;
  l3.SetLoopName("loop3");
  l3.Start();
  sleep(2);

  std::vector<base::MessageLoop2*> loops = {&loop, &l2, &l3};

  auto new_connection = [&](int fd, const net::InetAddress& peer) {

    base::MessageLoop2* loop = base::MessageLoop2::Current();
    LOG(INFO) << " New Connection from:" << peer.IpPortAsString() << " on loop:" << loop->LoopName();

    net::InetAddress local(net::socketutils::GetLocalAddrIn(fd));
    auto f = net::TcpChannel::Create(fd,
                                     local,
                                     peer,
                                     loop,
                                     net::ChannelServeType::kServerType);
    int32_t size = f->Send((const uint8_t*)"hello world", sizeof "hello world");
    LOG(INFO) << "N ByteWrite:" << size;
    f->Start();
    f->ShutdownChannel();
  };

  // same address can bind multi listener socket_fd

  net::InetAddress addr(5005);

  std::vector<net::RefServiceAcceptor> acceptors;

  for (base::MessageLoop2* l : loops) {

    l->PostTask(base::NewClosure([=, &acceptors]() {

      net::RefServiceAcceptor acceptor(new net::ServiceAcceptor(l->Pump(), addr));
      acceptor->SetNewConnectionCallback(std::bind(new_connection,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));
      acceptor->StartListen();
      acceptors.push_back(std::move(acceptor));

    }));
  }

  for (base::MessageLoop2* l : loops) {
    l->WaitLoopEnd();
  }
#endif
  return 0;
}
