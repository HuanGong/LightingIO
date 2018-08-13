
#include <vector>
#include "base/message_loop/message_loop.h"
#include "server/raw_server/raw_server.h"
#include "server/http_server/http_server.h"
#include "dispatcher/coro_dispatcher.h"
#include "coroutine/coroutine_scheduler.h"

using namespace net;
using namespace base;

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
  res->MutableBody() = "hello world";
}

void HandleRaw(const RawMessage* req, RawMessage* res) {
  LOG(INFO) << "Got A Http Message";
  res->SetCode(0);
  res->SetMethod(2);
}

int main() {
  std::vector<base::MessageLoop*> loops;
  for (int i = 0; i < 4; i++) {
    auto loop = new(base::MessageLoop);
    loop->Start();
    loops.push_back(loop);
  }

  CoroWlDispatcher* dispatcher = new CoroWlDispatcher(true);

  base::MessageLoop loop;
  loop.Start();

  net::HttpServer http_server;
  http_server.SetIoLoops(loops);
  http_server.SetDispatcher(dispatcher);
  http_server.ServeAddress("http://127.0.0.1:5006", std::bind(HandleHttp, std::placeholders::_1, std::placeholders::_2));

  net::RawServer raw_server;
  raw_server.SetIoLoops(loops);
  raw_server.SetDispatcher(dispatcher);
  raw_server.ServeAddress("raw://127.0.0.1:5005", std::bind(HandleRaw, std::placeholders::_1, std::placeholders::_2));

  loop.WaitLoopEnd();
}

