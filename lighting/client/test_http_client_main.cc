#include "base/message_loop/message_loop.h"

#include "unistd.h"
#include <iostream>
#include <list>

#include "httpchannel_libevent.h"
#include "base/time_utils.h"

std::vector<net::HttpChannelLibEvent*> client_channels;

void CreateHttpConnection() {
  std::cout << " make httpconnections cache" << std::endl;
  net::ChannelInfo info;
  info.host = "127.0.0.1";
  info.port = 6666;

  event_base* base = base::MessageLoop::Current()->EventBase();

  for (int i=0; i<50; i++) {
    client_channels.push_back(new net::HttpChannelLibEvent(info, base));
  }
}

void request_done_cb(evhttp_request* req, void* arg) {
  if (req == NULL) {
    printf("timed out!\n");
    return;
  } else if (req->response_code == 0) {
    printf("connection refused!\n");
    return;
  } else if (req->response_code != 200) {
    printf("error: %u %s\n", req->response_code, req->response_code_line);
    return;
  } else {
    printf("success: %u %s\n", req->response_code, req->response_code_line);
  }
  std::cout << "time end:" << base::time_ms() << std::endl;
  if (req->response_code == 200) {
    ev_ssize_t m_len = EVBUFFER_LENGTH(req->input_buffer);
    std::string response;
    response.reserve(m_len+1);

    response.assign((char*)EVBUFFER_DATA(req->input_buffer), m_len);
    response[m_len] = '\0';

    std::cout << "httprequest get response:" << response << std::endl;
  }
}

void connection_close_cb(evhttp_connection *con, void *arg) {
  printf(" >>>>>>>>>>> connection_close_cb \n");
}

void MakeHttpRequest() {
  static unsigned int round_robin = 0;
  if (client_channels.size() == 0) {
    std::cout << "no connection to server" << std::endl;
    return;
  }
  auto con = client_channels[(++round_robin)%50];

  if (!con->EvConnection()) {
    con->InitConnection();
  }

  struct evhttp_request* request = evhttp_request_new(request_done_cb,
                                                      con->EvConnection());

  evhttp_add_header(request->output_headers, "Host", "127.0.0.1");
  evhttp_add_header(request->output_headers, "Connection", "keep-alive");
  //evbuffer_add(request->output_buffer, "hello world", sizeof("hello world"));

  std::cout << "time start:" << base::time_ms() << std::endl;

  evhttp_cmd_type cmd = EVHTTP_REQ_GET;// : EVHTTP_REQ_GET;
  int ret = evhttp_make_request(con->EvConnection(),
                                request, cmd, "/");
  if (ret != 0) {
    std::cout << "make request failed " << std::endl;
    return;
  }
}


int main() {
  base::MessageLoop ioloop("io");
  ioloop.Start();
  ioloop.PostTask(base::NewClosure(std::bind(CreateHttpConnection)));
  std::cout << "start http reqeust after 2s" << std::endl;

  static long times = 100000;
  for (int i = 0; i < times; i++) {
    ioloop.PostTask(base::NewClosure(std::bind(MakeHttpRequest)));
    usleep(5000);
  }

  sleep(1);


  std::cout << " going to exit main" << std::endl;
  bool run = true;
  /*
  ioloop.PostTask(base::NewClosure(std::bind([&](){
    run = false;
  })));
  */
  while(run) {
    usleep(5000);
  }
  ioloop.PostTask(base::NewClosure([&]() {
    std::cout << "delete client channals:" << client_channels.size() << std::endl;
    for (auto v : client_channels) {
      delete v;
    }
    client_channels.clear();
  }));
  sleep(2);
}

