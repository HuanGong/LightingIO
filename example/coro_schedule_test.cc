#include "libcoro/coro.h"
#include "message_loop/message_loop.h"
#include "unistd.h"
#include <functional>
#include "coroutine/coroutine.h"
#include "coroutine/coroutine_task.h"
#include "lighting/client/httpchannel_libevent.h"

base::MessageLoop* io_ptr = nullptr;
base::MessageLoop* woker_ptr = nullptr;
unsigned int request_count = 0;
class HttpRequest {
public:
  int request_id;
  std::string response;
  base::MessageLoop* handler_worker;
};

class HttpMessage {
public:
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
  woker_main_coro->Transfer(httpreqcoro);
  LOG(INFO) << __FUNCTION__ << "Worker Thread alive again";
}

void CreateHttpConnection() {
  std::cout << " make a httpconnection " << std::endl;
  net::ChannelInfo info;
  info.host = "127.0.0.1";
  info.port = 6666;
  event_base* base = base::MessageLoop::Current()->EventBase();
  client_channel = new net::HttpChannelLibEvent(info, base);
}

void request_done_cb(evhttp_request* request, void* arg) {
  LOG(INFO) << "time end" << base::time_ms();
  HttpRequest* req = static_cast<HttpRequest*>(arg);

  ev_ssize_t m_len;
	struct evhttp_connection *evcon = evhttp_request_get_connection(request);

  m_len = EVBUFFER_LENGTH(request->input_buffer);

  req->response.reserve(m_len+1);
  req->response.assign((char*)EVBUFFER_DATA(request->input_buffer), m_len);
  req->response[m_len] = '\0';

  LOG(INFO) << "ready let http request coro resume";
  req->handler_worker->PostTask(
    base::NewClosure(std::bind(&ResumeHttpRequestCoro, req)));
  //free(m_response);
}

void MakeHttpRequest(HttpRequest* req) {
  if (client_channel == NULL) {
    std::cout << "no connection to server" << std::endl;
  }

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
  std::cout << "time start:" << base::time_ms() << std::endl;
  int ret = evhttp_make_request(client_channel->EvHttpConnection(),
                                request,
                                cmd,
                                "http://127.0.0.1:6666/post");
  if (ret != 0) {
    LOG(INFO) << "make request failed ";
    return;
  }
}

void HttpRequestCoro(std::shared_ptr<HttpMessage> msg) {
  LOG(INFO) << __FUNCTION__ << " CORO RUN Begain";
  HttpRequest request;
  request.request_id = ++request_count;
  request.handler_worker = base::MessageLoop::Current();

  msg->io->PostTask(base::NewClosure(std::bind(&MakeHttpRequest, &request)));
  httpreqcoro->Yield();

  LOG(INFO) << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! it's works!!!!!!!!!!!!!!!!!!";
  LOG(INFO) << __FUNCTION__ << " CORO RUN END, request response:" << request.response;
}

void CoroWokerHttpHandle(std::shared_ptr<HttpMessage> msg) {
  LOG(INFO) << " Handle http message on CORO";

  //do something speed 10ms
  //usleep(10000);
  // emulate a client request
  httpreqcoro = new base::Coroutine();
  httpreqcoro->SetCaller(woker_main_coro);
  std::unique_ptr<base::CoroTask>
    func(base::NewCoroTask(std::bind(&HttpRequestCoro, std::move(msg))));
  httpreqcoro->SetCoroTask(std::move(func));

  woker_ptr->PostTask(base::NewClosure([&](){
    LOG(INFO) << "schedule a nother coro run http request";
    woker_main_coro->Transfer(httpreqcoro);
  }));

  httphandle_coro->Yield();

  LOG(INFO) << " Handle httpmessage Done On CORO; 5 msg.user_count:" << msg.use_count();
}

void HandleHttpMsg(std::shared_ptr<HttpMessage> msg) {

  LOG(INFO) << " 3 msg.user_count:" << msg.use_count();

  httphandle_coro = new base::Coroutine();
  httphandle_coro->SetCaller(woker_main_coro);
  std::unique_ptr<base::CoroTask>
    handle_http(base::NewCoroTask(std::bind(&CoroWokerHttpHandle, std::move(msg))));
  httphandle_coro->SetCoroTask(std::move(handle_http));

  LOG(INFO) << " schedule a coro to handle httpmessage";
  LOG(INFO) << " 4 msg.user_count:" << msg.use_count();

  woker_main_coro->Transfer(httphandle_coro);

  LOG(INFO) << " 6 msg.user_count:" << msg.use_count();
  LOG(INFO) << "HttpMessage Handle finished";
}

int main(int arvc, char **argv) {

  base::Coroutine coro(0, true);

  base::MessageLoop loop("IO");
  base::MessageLoop worker("WORKER");
  loop.Start();
  worker.Start();
  io_ptr = &loop;
  woker_ptr = &worker;

  LOG(INFO) << "main thread" << std::this_thread::get_id();

  //init connect client
  loop.PostTask(base::NewClosure(std::bind(CreateHttpConnection)));

  //init coro_woker
  worker.PostTask(base::NewClosure([&]() {
    woker_main_coro = new base::Coroutine(0, true);
  }));

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

  //a fake httpmsg to worker
  while(1) {
#if 0
    loop.PostTask(base::NewClosure([&]() {
      LOG(INFO) << "IOloop alive";
      auto func = []() {
        LOG(INFO) << "worker alive";
      };
      worker.PostTask(base::NewClosure(func));
    }));
#endif
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
  }
}
