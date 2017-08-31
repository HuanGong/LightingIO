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

base::Coroutine* woker_main_coro;
base::Coroutine* httphandle_coro;
base::Coroutine* httpreqcoro;
net::HttpChannelLibEvent* client_channel = NULL;

void ResumeHttpRequestCoro(HttpRequest* request) {
  LOG(INFO) << __FUNCTION__ << "Resume this request coro here";
  base::Coroutine::Current()->Transfer(request->coro_task);
}

void CreateHttpConnection() {
  std::cout << " make a httpconnection " << std::endl;
  net::ChannelInfo info;
  info.host = "127.0.0.1";
  info.port = 6666;
  event_base* base = base::MessageLoop::Current()->EventBase();
  client_channel = new net::HttpChannelLibEvent(info, base);
}

//io
void request_done_cb(evhttp_request* request, void* arg) {
  LOG(INFO) << __FUNCTION__ << " Get Response From Server On IO";
  HttpRequest* req = static_cast<HttpRequest*>(arg);

  ev_ssize_t m_len;
	struct evhttp_connection *evcon = evhttp_request_get_connection(request);

  m_len = EVBUFFER_LENGTH(request->input_buffer);

  req->response.reserve(m_len+1);
  req->response.assign((char*)EVBUFFER_DATA(request->input_buffer), m_len);
  req->response[m_len] = '\0';

  LOG(INFO) << __FUNCTION__ << " Going to resume the coro which Send the This Request";
  req->handler_worker->PostTask(
    base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
}

void MakeHttpRequest(HttpRequest* req) {
  if (client_channel == NULL) {
    LOG(ERROR) << __FUNCTION__ << " no connection to server";
    return;
  }

  LOG(INFO) << __FUNCTION__ << " Start Http Request On IO";

  event_base* base = base::MessageLoop::Current()->EventBase();
  struct evhttp_request* request = evhttp_request_new(request_done_cb, req);
  evhttp_add_header(request->output_headers, "Connection", "keep-alive");
  evhttp_add_header(request->output_headers, "Host", "127.0.0.1");
  evbuffer_add(request->output_buffer, "hello world", sizeof("hello world"));

  if (client_channel->EvHttpConnection()) {
    event_base* base = base::MessageLoop::Current()->EventBase();
    event_base_loop(base, EVLOOP_NONBLOCK);
  }

  if (!client_channel->EvHttpConnection()) {
    client_channel->InitConnection();
  }

  evhttp_cmd_type cmd = EVHTTP_REQ_POST;// : EVHTTP_REQ_GET;
  int ret = evhttp_make_request(client_channel->EvHttpConnection(),
                                request,
                                cmd,
                                "http://127.0.0.1:6666/post");
  if (ret != 0) {
    LOG(INFO) << __FUNCTION__ << " make request failed ";
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

  LOG(INFO) << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! it's works!!!!!!!!!!!!!!!!!!";
  LOG(INFO) << __FUNCTION__ << " CORO RUN END, request response:" << request.response;
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
    usleep(10);
/*
    // make a fake io event create a fake httpmsg
    loop.PostTask(base::NewClosure([&]() {
      std::shared_ptr<HttpMessage> incoming_http(new HttpMessage());
      incoming_http->code = 200;
      incoming_http->content = "hello world";
      incoming_http->io = &loop;

      LOG(INFO) << " 1 msg.user_count:" << incoming_http.use_count();
      auto func = std::bind(&HandleHttpMsg, std::move(incoming_http));
      worker.PostTask(base::NewClosure(func));
      LOG(INFO) << " 2 msg.user_count:" << incoming_http.use_count();
    }));
    */
    if (i != 10000)
      continue;
    i = 0;
    worker.PostTask(base::NewClosure([]() {LOG(INFO) << "Work still Alive!";}));
  }
}
