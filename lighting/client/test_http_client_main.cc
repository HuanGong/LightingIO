#include "base/message_loop/message_loop.h"

#include "unistd.h"

#include "httpchannel_libevent.h"

HttpChannelLibEvent* client_channel = NULL;

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
  info.port = 8005;

  event_base* base = base::MessageLoop::Current()->EventBase();

  client_channel = new HttpChannelLibEvent(info, base);
}

void request_done_cb(evhttp_request* request, void* arg) {
  std::cout << "http request back" << std::endl;
	ev_ssize_t len;
	struct evhttp_connection *evcon = evhttp_request_get_connection(request);

	struct evbuffer *evbuf;
	evbuf = evhttp_request_get_input_buffer(req);

	len = evbuffer_get_length(evbuf);
	fwrite(evbuffer_pullup(evbuf, len), len, 1, stdout);
	evbuffer_drain(evbuf, len);
}

void MakeHttpRequest() {
  if (client_channel == NULL) {
    std::cout << "no connection to server" << std::endl;
  }

  struct evhttp_request* request = evhttp_request_new(request_done_cb, base);
  //evhttp_request_set_header_cb(request, ReadHeaderDoneCallback);
  //evhttp_request_set_chunked_cb(request, ReadChunkCallback);
  //evhttp_request_set_error_cb(request, RemoteRequestErrorCallback);

}


int main() {

  base::MessageLoop ioloop("io");
  ioloop.Start();



  while(1) {
    sleep(2);
  }
}
