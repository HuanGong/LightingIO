#ifndef HTTP_URL_REQUEST_CORO_JOB_H_
#define HTTP_URL_REQUEST_CORO_JOB_H_

#include <list>

#include "base/coroutine/coroutine.h"
#include "base/message_loop/message_loop.h"

#include "url_request_job.h"

namespace net {

class HttpUrlRequest;

class RequestCoroCtx {
public:
  RequestCoroCtx();
  ~RequestCoroCtx();
  bool ReadyToWork();
  base::Coroutine* coro_task_;
  base::MessageLoop* worker_loop_;
  base::MessageLoop* socket_loop_;_
};

//template<class T>
class RequestCoroJob : public UrlRequestJob {
public:
  static UrlRequestJob* Create();

  RequestCoroJob();
  ~RequestCoroJob();

  void AddUrlRequest(HttpUrlRequest* request);

  void SendRecieve() override;

private:
  bool InitJobContext();
  RequestCoroCtx request_coro_ctx_;
  std::list<HttpUrlRequest*> requests_;
};

} //end name space
#endif
