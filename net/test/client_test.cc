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

base::MessageLoop2 loop;
base::MessageLoop2 wloop;
net::InetAddress server_address("0.0.0.0", 5006);

net::ClientRouter*  router; //(base::MessageLoop2*, const InetAddress&);

net::RefHttpRequest g_request;
net::RefTcpChannel g_channel;

void SendRequest() {
  net::RefHttpRequest request = std::make_shared<net::HttpRequest>(net::IODirectionType::kOutRequest);
  request->SetKeepAlive(true);
  request->MutableBody() = "Nice to meet your,I'm LightingIO\n";

  net::RefProtocolMessage req = std::static_pointer_cast<net::ProtocolMessage>(request);

  if (router->SendClientRequest(req)) {
    LOG(ERROR) << "Haha, My Request Back ................. Wow!!!!";
  }
}

int main(int argc, char* argv[]) {

  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);
  dispatcher_->InitWorkLoop(4);
  dispatcher_->StartDispatcher();

  dispatcher_->InitWorkLoop(2);
  dispatcher_->StartDispatcher();

  loop.SetLoopName("clientloop");
  wloop.SetLoopName("workloop");

  loop.Start();
  wloop.Start();

  router = new net::ClientRouter(&loop, server_address);
  router->SetWorkLoadTransfer(dispatcher_);
  loop.PostTask(base::NewClosure(std::bind(&net::ClientRouter::StartRouter, router)));
  sleep(5);

  base::StlClosure closure = std::bind(SendRequest);
  base::CoroScheduler::RunAsCoroInLoop(&wloop, closure);

  base::CoroScheduler::RunAsCoroInLoop(&wloop, closure);

  loop.WaitLoopEnd();
  delete router;
  return 0;
}
