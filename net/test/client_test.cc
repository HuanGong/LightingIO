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

base::MessageLoop2 loop;
base::MessageLoop2 wloop;
net::InetAddress server_address("0.0.0.0", 5006);
net::InetAddress raw_server_address("0.0.0.0", 5002);

net::ClientRouter*  router; //(base::MessageLoop2*, const InetAddress&);
net::ClientRouter* raw_router;
net::RefHttpRequest g_request;
net::RefTcpChannel g_channel;

void SendRequest() {
  net::RefHttpRequest request = std::make_shared<net::HttpRequest>(net::IODirectionType::kOutRequest);
  request->SetKeepAlive(true);
  request->MutableBody() = "Nice to meet your,I'm LightingIO\n";

  net::RefProtocolMessage req = std::static_pointer_cast<net::ProtocolMessage>(request);

  if (router->SendClientRequest(req)) {
    LOG(ERROR) << "Haha, My HttpRequest Back ................. Wow!!!!";
  }
}

void SendRawRequest() {
  net::RefRawMessage raw_request =
    std::make_shared<net::RawMessage>(net::IODirectionType::kOutRequest);
  std::string content("this is a raw request");
  raw_request->SetContent(content);
  raw_request->SetCode(12);
  raw_request->SetMethod(12);
  raw_request->SetSequenceId(100);

  if (raw_router->SendRecieve<net::RefRawMessage>(raw_request)) {
    LOG(ERROR) << "Haha, My Raw Request Back ............. Wow!!!!";
  }
}

void SendRawRequest() {
  net::RefRawMessage raw_request =
    std::make_shared<net::RawMessage>(net::IODirectionType::kOutRequest);
  std::string content("this is a raw request");
  raw_request->SetContent(content);
  raw_request->SetCode(12);
  raw_request->SetMethod(12);
  raw_request->SetSequenceId(100);

  net::RefProtocolMessage req = std::static_pointer_cast<net::ProtocolMessage>(raw_request);
  if (raw_router->SendClientRequest(req)) {
    LOG(ERROR) << "Haha, My Raw Request Back ............. Wow!!!!";
  }
}

int main(int argc, char* argv[]) {

  google::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags

  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);
  dispatcher_->InitWorkLoop(4);
  dispatcher_->StartDispatcher();

  loop.SetLoopName("clientloop");
  wloop.SetLoopName("workloop");

  loop.Start();
  wloop.Start();

  raw_router = new net::ClientRouter(&loop, raw_server_address);
  net::RouterConf raw_router_config;
  raw_router_config.protocol = "raw";
  raw_router_config.connections = 2;
  raw_router_config.recon_interal = 5000;
  raw_router_config.message_timeout = 1000;
  raw_router->SetupRouter(raw_router_config);
  raw_router->SetWorkLoadTransfer(dispatcher_);
  loop.PostTask(base::NewClosure(std::bind(&net::ClientRouter::StartRouter, raw_router)));

#if 0
  router = new net::ClientRouter(&loop, server_address);
  router->SetWorkLoadTransfer(dispatcher_);
  loop.PostTask(base::NewClosure(std::bind(&net::ClientRouter::StartRouter, router)));
#endif

  sleep(5);
#if 0
  base::StlClosure closure = std::bind(SendRequest);
  base::CoroScheduler::RunAsCoroInLoop(&wloop, closure);
#endif

  base::StlClosure send_raw_request = std::bind(SendRawRequest);
  base::CoroScheduler::RunAsCoroInLoop(&wloop, send_raw_request);

  loop.WaitLoopEnd();
  delete router;
  delete raw_router;
  return 0;
}
