#include "base/message_loop/message_loop.h"

#include "unistd.h"
#include <iostream>

#include "httpchannel_libevent.h"
#include "base/time_utils.h"

net::HttpChannelLibEvent* client_channel = NULL;

/**
 * Creates a new request object that needs to be filled in with the request
 * parameters.  The callback is executed when the request completed or an
 * error occurred.
 */
//struct evhttp_request *evhttp_request_new(
//	void (*cb)(struct evhttp_request *, void *), void *arg);
void CreateHttpConnection() {

  std::cout << " make a httpconnection " << std::endl;
  net::ChannelInfo info;
  info.host = "127.0.0.1";
  info.port = 6666;

  event_base* base = base::MessageLoop::Current()->EventBase();

  client_channel = new net::HttpChannelLibEvent(info, base);
}

void request_done_cb(evhttp_request* request, void* arg) {
  std::cout << "time end:" << base::time_ms() << std::endl;
	ev_ssize_t m_len;
	struct evhttp_connection *evcon = evhttp_request_get_connection(request);

  m_len = EVBUFFER_LENGTH(request->input_buffer);
  char* m_response = (char*)malloc(m_len + 1);
  std::cout << "response length:" << m_len;

  strncpy(m_response, (char*)EVBUFFER_DATA(request->input_buffer), m_len);
  m_response[m_len] = '\0';
  std::cout << "http request get response:" << std::string(m_response) << std::endl;
}

void MakeHttpRequest() {
  if (client_channel == NULL) {
    std::cout << "no connection to server" << std::endl;
  }

  event_base* base = base::MessageLoop::Current()->EventBase();
  struct evhttp_request* request = evhttp_request_new(request_done_cb, base);
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
    std::cout << "make request failed " << std::endl;
    return;
  }
}


int main() {

  base::MessageLoop ioloop("io");
  ioloop.Start();

  ioloop.PostTask(base::NewClosure(std::bind(CreateHttpConnection)));

  ioloop.PostTask(base::NewClosure(std::bind(MakeHttpRequest)));

  while(1) {
    sleep(5);
    ioloop.PostTask(base::NewClosure(std::bind(MakeHttpRequest)));
  }
}

