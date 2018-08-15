#include <glog/logging.h>

#include <vector>
#include "../tcp_channel.h"
#include "../socket_utils.h"
#include "../service_acceptor.h"
#include "../protocol/proto_service.h"
#include "../protocol/line/line_message.h"
#include "../protocol/http/http_request.h"
#include "../protocol/http/http_response.h"
#include "../protocol/proto_service_factory.h"
#include "../inet_address.h"
#include "clients/client_connector.h"
#include "clients/client_router.h"
#include "clients/client_channel.h"
#include "dispatcher/coro_dispatcher.h"
#include "base/coroutine/coroutine_scheduler.h"
#include "base/closure/closure_task.h"
#include "net/dispatcher/coro_dispatcher.h"
#include "net/protocol/raw/raw_message.h"
#include "net/protocol/raw/raw_proto_service.h"

base::MessageLoop loop;
base::MessageLoop wloop;
net::InetAddress server_address("0.0.0.0", 5006);
net::InetAddress raw_server_address("0.0.0.0", 5002);

net::ClientRouter*  router; //(base::MessageLoop*, const InetAddress&);
net::ClientRouter* raw_router;
net::RefHttpRequest g_request;
net::RefTcpChannel g_channel;

void SendRequest(int sequence_id) {
  net::RefHttpRequest request = std::make_shared<net::HttpRequest>();
  request->SetKeepAlive(true);
  request->MutableBody() = "Nice to meet your,I'm LightingIO\n";

  net::RefProtocolMessage req = std::static_pointer_cast<net::ProtocolMessage>(request);

  if (router->SendClientRequest(req)) {
    LOG(ERROR) << "Haha, My HttpRequest Back .......... Wow!!!!" << " sequence id:" << sequence_id;
  }
}

void SendRawRequest(int sequence_id) {
  net::RefRawMessage raw_request = net::RawMessage::CreateRequest();
  std::string content("this is a raw request");
  raw_request->SetContent(content);
  raw_request->SetCode(12);
  raw_request->SetMethod(12);
  //raw_request->SetSequenceId(sequence_id);

  if (raw_router->SendRecieve<net::RefRawMessage>(raw_request)) {
    //LOG(ERROR) << "Haha, My Raw Request Back ............. Wow!!!!";
    auto& response = raw_request->Response();
    if (response) {
      net::RawMessage* raw_response = static_cast<net::RawMessage*>(response.get());
      LOG(ERROR) << "Get RawResponse:\n" << raw_response->MessageDebug();
    }
  }
}

int main(int argc, char* argv[]) {

  google::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags

  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);

  loop.SetLoopName("clientloop");
  wloop.SetLoopName("workloop");

  loop.Start();
  wloop.Start();

  router = new net::ClientRouter(&loop, server_address);
  net::RouterConf router_config;
  router_config.protocol = "http";
  router_config.connections = 1;
  router_config.recon_interal = 5000;
  router_config.message_timeout = 1000;
  router->SetupRouter(router_config);
  router->SetWorkLoadTransfer(dispatcher_);
  loop.PostTask(base::NewClosure(std::bind(&net::ClientRouter::StartRouter, router)));

  sleep(5);

  for (int i = 0; i < 10; i++) {
    base::StlClosure send_request = std::bind(SendRequest, i);
    loop.PostCoroTask(send_request);
  }

  loop.WaitLoopEnd();
  delete router;
  delete raw_router;
  return 0;
}
