#include "http_request_job.h"
#include "httpchannel_libevent.h"

#include "base/coroutine/coroutine.h"
#include "base/coroutine/coroutine_scheduler.h"

namespace net {

RequestCoroCtx::RequestCoroCtx()
  : coro_task_(nullptr),
    worker_loop_(nullptr),
    socket_loop_(nullptr) {
}

RequestCoroCtx::~RequestCoroCtx() {
  coro_task_ = nullptr;
  worker_loop_ = nullptr;
  socket_loop_ = nullptr;
}

bool RequestCoroCtx::ReadyToWork() {
  return coro_task_ != nullptr &&
         worker_loop_ != nullptr &&
         socket_loop_ != nullptr;
}

//template<class T>
//class RequestCoroJob : public UrlRequestJob {
//static
UrlRequestJob* RequestCoroJob::Create() {
  RequestCoroJob* job = new RequestCoroJob();
  job->InitJobContext();
}

bool RequestCoroJob::InitJobContext() {
  request_coro_ctx_.coro_task_ = base::Coroutine::Current();
  request_coro_ctx_.worker_loop_ = base::MessageLoop::TlsCurrent();
  return true;
}

RequestCoroJob::RequestCoroJob() {
}

RequestCoroJob::~RequestCoroJob() {
}

void UrlRequestJob::AddUrlRequest(HttpUrlRequest* request) {
  if (!request) {
    return;
  }
  requests_.push_back(request);
}

void UrlRequestJob::SendRecieve() override {

}



}//end net
