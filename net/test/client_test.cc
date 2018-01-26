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

base::MessageLoop2 loop;
base::MessageLoop2 wloop;
net::InetAddress server_address("0.0.0.0", 5006);

net::ClientRouter*  router; //(base::MessageLoop2*, const InetAddress&);

net::RefHttpRequest g_request;
net::RefTcpChannel g_channel;

void SendRequest() {
  net::RefHttpRequest request = std::make_shared<net::HttpRequest>(net::IODirectionType::kOutRequest);
  request->SetKeepAlive(false);
  request->MutableBody() = "Nice to meet your,I'm LightingIO\n";

  net::RefProtocolMessage req = std::static_pointer_cast<net::ProtocolMessage>(request);

  if (router->SendClientRequest(req)) {
    base::CoroScheduler::TlsCurrent();
    base::CoroScheduler::TlsCurrent()->YieldCurrent();
    LOG(ERROR) << "Haha, My Request Back ................. Wow!!!!";
  }
}

int main(int argc, char* argv[]) {

  loop.SetLoopName("clientloop");
  loop.Start();
  wloop.SetLoopName("workloop");
  wloop.Start();

  router = new net::ClientRouter(&loop, server_address);
  loop.PostTask(base::NewClosure(std::bind(&net::ClientRouter::StartRouter, router)));
  sleep(5);

  wloop.PostTask(base::NewClosure([&](){
    base::CoroScheduler::CreateAndSchedule(base::NewCoroTask(std::bind(&SendRequest)));
    LOG(ERROR) << "Send Request Finished Or Yiled";
  }));

  loop.WaitLoopEnd();
  delete router;
  return 0;
}
