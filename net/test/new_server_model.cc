
#include <vector>
#include "base/message_loop/message_loop.h"
#include "protocol/redis/redis_request.h"
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
net::ClientRouter*  redis_router;
void StartRedisClient() {
  {
    net::InetAddress server_address("127.0.0.1", 6379);
    redis_router = new net::ClientRouter(&main_loop, server_address);
    net::RouterConf router_config;
    router_config.protocol = "redis";
    router_config.connections = 2;
    router_config.recon_interal = 5000;
    router_config.message_timeout = 1000;
    redis_router->SetupRouter(router_config);
    redis_router->SetWorkLoadTransfer(dispatcher_);
    redis_router->StartRouter();
  }
}
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

void DumpRedisResponse(RedisResponse* redis_response) {
  for (size_t i = 0; i < redis_response->Count(); i++) {
    auto& value = redis_response->ResultAtIndex(i);
    switch(value.type()) {
      case resp::ty_string: {
        LOG(INFO) << "string:" << value.string().data();
      }
      break;
      case resp::ty_error: {
        LOG(INFO) << "error:" << value.error().data();
      }
      break;
      case resp::ty_integer: {
        LOG(INFO) << "iterger:" << value.integer();
      }
      break;
      case resp::ty_null: {
        LOG(INFO) << "null:";
      } break;
      case resp::ty_array: {
        resp::unique_array<resp::unique_value> arr = value.array();
        std::ostringstream oss;
        for (size_t i = 0; i < arr.size(); i++) {
          oss << "'" << std::string(arr[i].bulkstr().data(), arr[i].bulkstr().size()) << "'";
          if (i < arr.size() - 1) {
            oss << ", ";
          }
        }
        LOG(INFO) << "array: [" << oss.str() << "]";
      } break;
      case resp::ty_bulkstr: {
        LOG(INFO) << "bulkstr:" << std::string(value.bulkstr().data(), value.bulkstr().size());
      }
      break;
      default: {
        LOG(INFO) << " default handler for redis message response";
      }
      break;
    }
  }
}

void SendRedisMessage() {
  auto redis_request = std::make_shared<RedisRequest>();

  redis_request->SetWithExpire("name", "huan.gong", 2000);
  redis_request->Exists("name");
  redis_request->Delete("name");
  /*
  redis_request->Incr("counter");
  redis_request->IncrBy("counter", 10);
  redis_request->Decr("counter");
  redis_request->DecrBy("counter", 10);
  */

  redis_request->Select("1");
  redis_request->Auth("");

  redis_request->TTL("counter");
  redis_request->Expire("counter", 200);
  redis_request->Persist("counter");
  redis_request->TTL("counter");

  auto redis_response  = redis_router->SendRecieve(redis_request);
  if (redis_response) {
    LOG(INFO) << "redis client got response:" << redis_response->Count();
    DumpRedisResponse(redis_response);
  } else {
    LOG(ERROR) << "redis client request failed:" << redis_request->FailMessage();
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

#if 0
  StartHttpClients();
#endif
#if 0
  std::this_thread::sleep_for(std::chrono::seconds(5));
  for (int i = 0; i < 10; i++) {
    main_loop.PostCoroTask(std::bind(SendHttpRequest));
  }
#endif

#if 0
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

  StartRedisClient();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  main_loop.PostCoroTask(std::bind(SendRedisMessage));

  main_loop.WaitLoopEnd();
}

