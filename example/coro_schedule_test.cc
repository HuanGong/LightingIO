#include "libcoro/coro.h"
#include "message_loop/message_loop.h"
#include "unistd.h"
#include <functional>
#include "coroutine/coroutine.h"
#include "coroutine/coroutine_task.h"
#include "coroutine/coroutine_scheduler.h"
#include "lighting/client/httpchannel_libevent.h"

base::MessageLoop* io_ptr = nullptr;
base::MessageLoop* woker_ptr = nullptr;
unsigned int request_count = 0;
class HttpRequest {
public:
  ~HttpRequest() {
    LOG(INFO) << __FUNCTION__ << " HttpRequest deleted >>>>>>>>>>>";
  }
  int request_id;
  std::string response;
  base::MessageLoop* handler_worker;
  base::Coroutine* coro_task;
};
class HttpMessage {
public:
  ~HttpMessage() {
    LOG(INFO) << __FUNCTION__ << " HttpMessage deleted >>>>>>>>>>>";
  }
  int code;
  int a[1024];
  std::string content;
  base::MessageLoop* io;
};
struct RequestContex {
  HttpRequest* req;
  net::HttpChannelLibEvent* connection;
};

std::list<net::HttpChannelLibEvent*> client_channels;

void ResumeHttpRequestCoro(HttpRequest* request) {
  LOG(INFO) << __FUNCTION__ << "Resume this request coro here";
  base::Coroutine::Current()->Transfer(request->coro_task);
}

void CreateHttpConnection() {
  LOG(INFO) << __FUNCTION__ << " make a httpconnection ";
  net::ChannelInfo info;
  info.host = "127.0.0.1";
  info.port = 6666;
  event_base* base = base::MessageLoop::Current()->EventBase();
  int n = 50;
  for (int n = 50; n > 0; n--) {
    client_channels.push_back(new net::HttpChannelLibEvent(info, base));
  }
}

//io
void request_done_cb(evhttp_request* request, void* arg) {
  LOG(INFO) << __FUNCTION__ << " Get Response From Server On IO";
  //HttpRequest* req = static_cast<HttpRequest*>(arg);
  RequestContex* rctx = static_cast<RequestContex*>(arg);
  HttpRequest* req = rctx->req;
  net::HttpChannelLibEvent* connection = rctx->connection;

  ev_ssize_t m_len;
	struct evhttp_connection *evcon = evhttp_request_get_connection(request);

  m_len = EVBUFFER_LENGTH(request->input_buffer);

  req->response.reserve(m_len+1);
  req->response.assign((char*)EVBUFFER_DATA(request->input_buffer), m_len);
  req->response[m_len] = '\0';

  LOG(INFO) << __FUNCTION__ << " Going to resume the coro which Send the This Request";
  req->handler_worker->PostTask(
    base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
  delete rctx;
  client_channels.push_front(connection);
}

void MakeHttpRequest(HttpRequest* req) {
  if (!client_channels.size()) {
    LOG(ERROR) << __FUNCTION__ << " no connection to server";
    req->response = "========= no clients ===========";
    req->handler_worker->PostTask(
      base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
    return;
  }

  LOG(INFO) << __FUNCTION__ << " Start Http Request On IO";
  RequestContex* rctx = new RequestContex();
  rctx->connection = client_channels.back();
  client_channels.pop_back();
  rctx->req = req;

  event_base* base = base::MessageLoop::Current()->EventBase();
  struct evhttp_request* request = evhttp_request_new(request_done_cb, rctx);
  evhttp_add_header(request->output_headers, "Connection", "keep-alive");
  evhttp_add_header(request->output_headers, "Host", "127.0.0.1");
  evbuffer_add(request->output_buffer, "hello world", sizeof("hello world"));

  if (rctx->connection->EvHttpConnection()) {
    event_base* base = base::MessageLoop::Current()->EventBase();
    event_base_loop(base, EVLOOP_NONBLOCK);
  }

  if (!rctx->connection->EvHttpConnection()) {
    rctx->connection->InitConnection();
  }

  evhttp_cmd_type cmd = EVHTTP_REQ_POST;// : EVHTTP_REQ_GET;
  int ret = evhttp_make_request(rctx->connection->EvHttpConnection(),
                                request,
                                cmd,
                                "http://127.0.0.1:6666/post");
  if (ret != 0) {
    LOG(INFO) << __FUNCTION__ << " make request failed ";
    req->handler_worker->PostTask(
      base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
  }
}

void HttpRequestCoro(std::shared_ptr<HttpMessage> msg) {
  LOG(INFO) << __FUNCTION__ << " make http request on worker";
  HttpRequest request;
  request.request_id = ++request_count;
  request.handler_worker = base::MessageLoop::Current();
  request.coro_task = base::Coroutine::Current();

  msg->io->PostTask(base::NewClosure(std::bind(&MakeHttpRequest, &request)));
  base::Coroutine::Current()->Yield();

  LOG(INFO) << __FUNCTION__ << " request response:" << request.response;
}

void CoroWokerHttpHandle(std::shared_ptr<HttpMessage> msg) {
  LOG(INFO) << __FUNCTION__ << " Handle http message on CORO";

  auto this_coro = base::Coroutine::Current();

  auto coro = base::CoroScheduler::CreateAndSchedule(
    base::NewCoroTask(std::bind(&HttpRequestCoro, std::move(msg))));

  coro->SetSuperior(this_coro);
  LOG(INFO) << __FUNCTION__ << " Paused Here";
  this_coro->Yield();
}

void HandleHttpMsg(std::shared_ptr<HttpMessage> msg) {

  base::CoroScheduler::CreateAndSchedule(
    base::NewCoroTask(std::bind(&CoroWokerHttpHandle, std::move(msg))));
}

int main(int arvc, char **argv) {

  base::Coroutine coro(0, true);

  base::MessageLoop loop("IO");
  base::MessageLoop worker("WORKER");
  loop.Start(); worker.Start();
  io_ptr = &loop; woker_ptr = &worker;

  LOG(INFO) << "main thread" << std::this_thread::get_id();
  //init connect client
  loop.PostTask(base::NewClosure(std::bind(CreateHttpConnection)));

  //init coro_woker
  worker.PostTask(base::NewClosure([&]() {
    base::CoroScheduler::TlsCurrent();
  }));

  sleep(10);

  loop.PostTask(base::NewClosure([&]() {
    std::shared_ptr<HttpMessage> incoming_http(new HttpMessage());
    incoming_http->code = 200;
    incoming_http->io = &loop;
    incoming_http->content = "hello world";

    auto handle_in_works = std::bind(&HandleHttpMsg, std::move(incoming_http));
    worker.PostTask(base::NewClosure(handle_in_works));
  }));

  //a fake httpmsg to worker
  long i = 0;
  while(1) {
    i++;
    usleep(1000);
    //sleep(10);

    if (i <= 100) {
      loop.PostTask(base::NewClosure([&]() {
        std::shared_ptr<HttpMessage> incoming_http(new HttpMessage());
        incoming_http->code = 200;
        incoming_http->io = &loop;
        incoming_http->content = "hello world";

        auto handle_in_works = std::bind(&HandleHttpMsg, std::move(incoming_http));
        worker.PostTask(base::NewClosure(handle_in_works));
      }));
    }
    worker.PostTask(base::NewClosure([]() {LOG(INFO) << "Work still Alive!";}));
  }
}
