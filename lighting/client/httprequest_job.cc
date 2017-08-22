#include "http_request_job.h"
#include "httpchannel_libevent.h"

namespace net {

void RequestResponseCallback(evhttp_request* request, void* arg) {
}

HttpUrlRequestJob::HttpUrlRequestJob() {
}
HttpUrlRequestJob::~HttpUrlRequestJob() {
}

void HttpUrlRequestJob::Start() {
  struct evhttp_request* request = evhttp_request_new(RemoteReadCallback, base);
  //myself httprequest to evhttprequest
  struct evhttp_connection* connection = NULL;

}

}//end net
