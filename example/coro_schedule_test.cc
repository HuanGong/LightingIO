#include "libcoro/coro.h"
#include "message_loop/message_loop.h"
#include "unistd.h"
#include <atomic>
#include <functional>
#include "coroutine/coroutine.h"
#include "coroutine/coroutine_task.h"
#include "coroutine/coroutine_scheduler.h"
#include "lighting/client/httpchannel_libevent.h"

static std::atomic<int> request_count;
static std::atomic<int> resume_count;

class HttpRequest {
public:
  int request_id;
  std::string response;
  base::MessageLoop* handler_worker;
  base::Coroutine* coro_task;
};
class HttpMessage {
public:
  int code;
  std::string content;
  base::MessageLoop* io;
};

std::vector<net::HttpChannelLibEvent*> client_channels;

void ResumeHttpRequestCoro(HttpRequest* request) {
  base::Coroutine::Current()->Transfer(request->coro_task);
}

void CreateHttpConnection() {
}

void request_done_cb(evhttp_request* request, void* arg) {
  if (arg == NULL) {
    LOG(ERROR) << __FUNCTION__ << " Check Arg Failed";
    return;
  }
  if (request == NULL) {
    LOG(ERROR) << __FUNCTION__ << " Check request Failed";
    HttpRequest* req = static_cast<HttpRequest*>(arg);
    req->handler_worker->PostTask(
      base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
    return;
  }
  HttpRequest* req = static_cast<HttpRequest*>(arg);

  ev_ssize_t m_len = EVBUFFER_LENGTH(request->input_buffer);
  if (m_len) {
    req->response.reserve(m_len+1);
    req->response.assign((char*)EVBUFFER_DATA(request->input_buffer), m_len);
    req->response[m_len] = '\0';
  }
  req->handler_worker->PostTask(
    base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
}

void MakeHttpRequest(HttpRequest* req) {
  if (!client_channels.size()) {
    req->response = "ERROR";
    req->handler_worker->PostTask(
      base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
    return;
  }

  auto conn = client_channels[(request_count)%50];

  struct evhttp_request* request = evhttp_request_new(request_done_cb, req);

  evhttp_add_header(request->output_headers, "Host", "127.0.0.1");
  evhttp_add_header(request->output_headers, "Connection", "keep-alive");
  //evbuffer_add(request->output_buffer, "hello world", sizeof("hello world"));

  if (!conn->EvConnection()) {
    conn->InitConnection();
  }

  evhttp_cmd_type cmd = EVHTTP_REQ_GET;// : EVHTTP_REQ_GET;

  int ret = evhttp_make_request(conn->EvConnection(), request, cmd, "/");
  if (ret != 0) {
    LOG(ERROR) << "evhttp_make_reqeust failed";
    req->handler_worker->PostTask(
      base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
  }
}

void HttpRequestCoro(std::shared_ptr<HttpMessage> msg) {

  HttpRequest request;
  request.request_id = 0;
  request.handler_worker = base::MessageLoop::Current();
  request.coro_task = base::Coroutine::Current();

  LOG(INFO) << "make httprequest, request_count:" << (request_count++);

  msg->io->PostTask(base::NewClosure(std::bind(&MakeHttpRequest, &request)));
  base::Coroutine::Current()->Yield();
  resume_count++;
  LOG(INFO) << "httprequest resumed" << "get response:" << request.response;
}

void CoroWokerHttpHandle(std::shared_ptr<HttpMessage> msg) {
  auto this_coro = base::Coroutine::Current();
  auto coro = base::CoroScheduler::CreateAndSchedule(
    base::NewCoroTask(std::bind(&HttpRequestCoro, std::move(msg))));

  coro->SetSuperior(this_coro);
  this_coro->Yield();
  LOG(INFO) << "Httprequest from IO handle Finished";
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

  LOG(INFO) << "main thread" << std::this_thread::get_id();
  //init connect client
  loop.PostTask(base::NewClosure(std::bind([&](){
    net::ChannelInfo info;
    info.host = "127.0.0.1";
    info.port = 6666;
    event_base* base = base::MessageLoop::Current()->EventBase();
    for (int n = 0; n < 50; n++) {
      client_channels.push_back(new net::HttpChannelLibEvent(info, base));
    }
  })));

  LOG(INFO) << "Start Run Tests";
  sleep(2);

  //a fake httpmsg to worker
  long fake_httpmessage_count = 5000;
  for (; fake_httpmessage_count > 0; fake_httpmessage_count--) {
    loop.PostTask(base::NewClosure([&]() {
      std::shared_ptr<HttpMessage> incoming_http(new HttpMessage());
      incoming_http->code = 200;
      incoming_http->io = &loop;
      incoming_http->content = "hello world";

      auto handle_in_works = std::bind(&HandleHttpMsg, std::move(incoming_http));
      worker.PostTask(base::NewClosure(handle_in_works));
    }));
    usleep(5000);
  }

  bool running = true;
  worker.PostTask(base::NewClosure([&]() {
    running = false;
  }));

  while(running) {
    sleep(5);
  }
  sleep(100);
  for (auto v: client_channels) {
    delete v;
  }
  LOG(INFO) << "resume_count:" << resume_count << " request_count:" << request_count;
}
