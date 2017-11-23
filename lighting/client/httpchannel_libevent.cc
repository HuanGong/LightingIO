
#include "httpchannel_libevent.h"

#include "glog/logging.h"
#include "base/time/time_utils.h"

namespace net {

//static bridge
void HttpChannelLibEvent::on_connection_reseted(struct evhttp_connection* conn, void* obj) {
  CHECK(obj);
  HttpChannelLibEvent* channel = static_cast<HttpChannelLibEvent*>(obj);
  channel->OnChannelReseted();
}

HttpChannelLibEvent::HttpChannelLibEvent(ChannelInfo info, event_base* base)
  : ev_base_(base),
    evdns_base_(nullptr),
    connection_(nullptr),
    channel_info_(info),
    timeout_ms_(0),
    retry_max_(3),
    retry_interval_(1) {
	evdns_base_ = evdns_base_new(base, EVDNS_BASE_DISABLE_WHEN_INACTIVE);
}

HttpChannelLibEvent::~HttpChannelLibEvent() {
  CloseConnection();
}

void HttpChannelLibEvent::InitConnection() {
  if (connection_) {
    return;
  }

  if (connection_ == nullptr) {
    connection_ = evhttp_connection_base_new(ev_base_,
                                             evdns_base_,
                                             channel_info_.host.c_str(),
                                             channel_info_.port);
  }

  if (!connection_) {
    LOG(ERROR) << "HttpClient Init Failed in evhttp_connection_base_new";
    return;
  }
  //evhttp_connection_set_retries(connection_, retry_max_);
  //evhttp_connection_set_closecb(connection_, on_connection_reseted, this);

  //struct timeval retry_tv = base::ms_to_timeval(retry_interval_);
  //evhttp_connection_set_initial_retry_tv(connection_, &retry_tv);

  if (TimeoutMs() != 0) {
    struct timeval tv = base::ms_to_timeval(TimeoutMs());
    evhttp_connection_set_timeout_tv(connection_, &tv);
  }
}

void HttpChannelLibEvent::CloseConnection() {
  if (connection_) {
    evhttp_connection_free(connection_);
    connection_ = nullptr;
  }
  if (evdns_base_) {
    evdns_base_free(evdns_base_, 0);
    evdns_base_ = nullptr;
  }
}

void HttpChannelLibEvent::OnChannelReseted() {
  LOG(INFO) << "evhttp_connection was reseted";
  //notify others
  //delegate->OnChannelClosed(this);
}

ChannelStatus HttpChannelLibEvent::Status() {
  return ChannelStatus::ST_IDLE;
}

}// end namespace
