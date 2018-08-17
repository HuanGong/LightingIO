
#include <vector>
#include "base/message_loop/message_loop.h"
#include "server/raw_server/raw_server.h"
#include "server/http_server/http_server.h"
#include "dispatcher/coro_dispatcher.h"
#include "coroutine/coroutine_scheduler.h"

#include "clients/client_connector.h"
#include "clients/client_router.h"
#include "clients/client_channel.h"

using namespace net;
using namespace base;

void SendRawRequest();

base::MessageLoop main_loop;
std::vector<base::MessageLoop*> loops;
std::vector<base::MessageLoop*> workers;

CoroWlDispatcher* dispatcher_ = new CoroWlDispatcher(true);

void HandleHttp(const HttpRequest* req, HttpResponse* res) {
  LOG(INFO) << "Got A Http Message";


  auto loop = base::MessageLoop::Current();
  auto coro = base::CoroScheduler::CurrentCoro();
     
  //broadcast
  for (int i = 0; i < 10; i++) {
    loop->PostCoroTask(std::bind([&]() {
      SendRawRequest();
      coro->Resume(); 
    }));
  }

  int back = 0;
  while(back < 10) {
    base::CoroScheduler::TlsCurrent()->YieldCurrent(); 
    back++;
  }

  res->SetResponseCode(200);
  res->MutableBody() = "hello world";
}

void HandleRaw(const RawMessage* req, RawMessage* res) {
  LOG(INFO) << "Got A Raw Message";
  res->SetCode(0);
  res->SetMethod(2);
  res->SetContent("Raw Message");
}

net::ClientRouter*  raw_router; //(base::MessageLoop*, const InetAddress&);
net::ClientRouter*  http_router; //(base::MessageLoop*, const InetAddress&);
void StartRawClient() {
  {
    net::InetAddress server_address("0.0.0.0", 5005);
    raw_router = new net::ClientRouter(&main_loop, server_address);
    net::RouterConf router_config;
    router_config.protocol = "raw";
    router_config.connections = 2;
    router_config.recon_interal = 5000;
    router_config.message_timeout = 1000;
    raw_router->SetupRouter(router_config);
    raw_router->SetWorkLoadTransfer(dispatcher_);
    raw_router->StartRouter();
  }
}

void StartHttpClients() {
  {
    net::InetAddress server_address("0.0.0.0", 5006);
    http_router = new net::ClientRouter(&main_loop, server_address);
    net::RouterConf router_config;
    router_config.protocol = "http";
    router_config.connections = 2;
    router_config.recon_interal = 5000;
    router_config.message_timeout = 1000;
    http_router->SetupRouter(router_config);
    http_router->SetWorkLoadTransfer(dispatcher_);
    http_router->StartRouter();
  }
}

void SendHttpRequest() {
  auto http_request = std::make_shared<HttpRequest>();
  http_request->SetMethod("GET");
  http_request->SetRequestURL("/abc/list");
  http_request->SetKeepAlive(true);

  http_router->SendRecieve(http_request);
  auto http_response  = (HttpResponse*)http_request->Response().get();
  if (http_response) {
    LOG(INFO) << "http client got response:" << http_response->Body();
  } else {
    LOG(ERROR) << "http client request failed:" << http_request->FailMessage();
  }
}

void SendRawRequest() {
  auto raw_request = std::make_shared<RawMessage>();
  raw_request->SetMethod(1);
  raw_request->SetContent("ABC");

  raw_router->SendRecieve(raw_request);
  auto raw_response  = (RawMessage*)raw_router->SendRecieve(raw_request);
  if (raw_response) {
    LOG(INFO) << "raw client got response:" << raw_response->Content();
  } else {
    LOG(ERROR) << "raw client request failed:" << raw_request->FailMessage();
  }
}

void PrepareLoops(uint32_t io_count, uint32_t worker_count) {
  for (uint32_t i = 0; i < io_count; i++) {
    auto loop = new(base::MessageLoop);
    loop->Start();
    loops.push_back(loop);
  }

  for (uint32_t i = 0; i < worker_count; i++) {
    auto worker = new(base::MessageLoop);
    worker->Start();
    workers.push_back(worker);
  }
}

int main(int argc, char* argv[]) {

  google::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags
  main_loop.Start();

  PrepareLoops(std::thread::hardware_concurrency(), 2);
  dispatcher_->SetWorkerLoops(workers);

  net::RawServer raw_server;
  raw_server.SetIoLoops(loops);
  raw_server.SetDispatcher(dispatcher_);
  raw_server.ServeAddressSync("raw://127.0.0.1:5005", std::bind(HandleRaw, std::placeholders::_1, std::placeholders::_2));


  net::HttpServer http_server;
  http_server.SetIoLoops(loops);
  http_server.SetDispatcher(dispatcher_);
  http_server.ServeAddressSync("http://127.0.0.1:5006", std::bind(HandleHttp, std::placeholders::_1, std::placeholders::_2));

#if 1
  StartHttpClients();
#endif
#if 0
  std::this_thread::sleep_for(std::chrono::seconds(5));
  for (int i = 0; i < 10; i++) {
    main_loop.PostCoroTask(std::bind(SendHttpRequest));
  }
#endif

#if 1
  StartRawClient();
#endif
#if 0
  for (int i = 0; i < 10; i++) {
    main_loop.PostCoroTask(std::bind(SendRawRequest));
  }
#endif

#if 0
  std::this_thread::sleep_for(std::chrono::seconds(5));
  http_server.StopServerSync();
  raw_router->StopRouter();
#endif

  main_loop.WaitLoopEnd();
}

