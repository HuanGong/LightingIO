
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

base::MessageLoop main_loop;
std::vector<base::MessageLoop*> loops;
CoroWlDispatcher* dispatcher_ = new CoroWlDispatcher(true);

void HandleHttp(const HttpRequest* req, HttpResponse* res) {
  LOG(INFO) << "Got A Http Message";
  res->SetResponseCode(200);
  /*
     base::MessageLoop* l = base::MessageLoop::Current();
     RefCoroutine coro = CoroScheduler::CurrentCoro();
     l->PostDelayTask(NewClosure([&](){
     coro->Resume();
     }), 1000);
     LOG(INFO) << "Request Handler Yield";
     CoroScheduler::TlsCurrent()->YieldCurrent();
     LOG(INFO) << "Request Handler Resumed";
     */
  std::this_thread::sleep_for(std::chrono::seconds(2));
  res->MutableBody() = "hello world";
}

void HandleRaw(const RawMessage* req, RawMessage* res) {
  LOG(INFO) << "Got A Http Message";
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
  HttpResponse* http_response  = (HttpResponse*)http_request->Response().get();
  if (http_response) {
    LOG(INFO) << "http client got response:" << http_response->Body();
  } else {
    LOG(ERROR) << "http client request failed:" << http_request->FailMessage();
  }
}

void SendRawRequest() {
  auto raw_request = std::make_shared<RawMessage>();
  raw_request->SetMethod(1);
  //raw_request->SetSequenceId(1);
  raw_request->SetContent("ABC");

  raw_router->SendRecieve(raw_request);
  RawMessage* raw_response  = (RawMessage*)raw_request->Response().get();
  if (raw_response) {
    LOG(INFO) << "raw client got response:" << raw_response->Content();
  } else {
    LOG(ERROR) << "raw client request failed:" << raw_request->FailMessage();
  }
}

int main(int argc, char* argv[]) {

  google::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags

  main_loop.Start();

  for (int i = 0; i < 1; i++) {
    auto loop = new(base::MessageLoop);
    loop->Start();
    loops.push_back(loop);
  }

  //net::RawServer raw_server;
  //raw_server.SetIoLoops(loops);
  //raw_server.SetDispatcher(dispatcher_);
  //raw_server.ServeAddressSync("raw://127.0.0.1:5005", std::bind(HandleRaw, std::placeholders::_1, std::placeholders::_2));
  //StartRawClients();

  net::HttpServer http_server;
  http_server.SetIoLoops(loops);
  http_server.SetDispatcher(dispatcher_);
  http_server.ServeAddressSync("http://127.0.0.1:5006", std::bind(HandleHttp, std::placeholders::_1, std::placeholders::_2));

  StartHttpClients();

  std::this_thread::sleep_for(std::chrono::seconds(5));
  //main_loop.PostCoroTask(std::bind(SendRawRequest));
  main_loop.PostCoroTask(std::bind(SendHttpRequest));
  /*
  for (int i = 0; i < 3; i++) {
    main_loop.PostCoroTask(std::bind(SendRawRequest));
    main_loop.PostCoroTask(std::bind(SendHttpRequest));
  }*/
  std::this_thread::sleep_for(std::chrono::seconds(5));

  //http_server.StopServerSync();
  //raw_router->StopRouter();

  main_loop.WaitLoopEnd();
}

