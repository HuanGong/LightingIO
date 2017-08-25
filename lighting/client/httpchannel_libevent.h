#ifndef HTTP_CLIENT_CHANNEL_LIBEVENT_H
#define HTTP_CLIENT_CHANNEL_LIBEVENT_H

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include <event2/http_compat.h>
#include <event2/http_struct.h>
#include <event2/event_compat.h>
#include <event2/dns.h>
#include <event2/dns_compat.h>
#include <event2/dns_struct.h>
#include <event2/listener.h>

#include <string>

namespace net {

struct ChannelInfo {
  ChannelInfo()
    : port(-1) {
  }
  int port;
  std::string host;
};
// a mapping status to libevent evhttp_connection
enum ChannelStatus {
	ST_DISCONNECTED,	/**< not currently connected not trying either*/
	ST_CONNECTING,	  /**< tries to currently connect */
	ST_IDLE,		      /**< connection is established */
	ST_READING_FIRSTLINE,/**< reading Request-Line (incoming conn) or
	                      **< Status-Line (outgoing conn) */
	ST_READING_HEADERS,	/**< reading request/response headers */
	ST_READING_BODY,	/**< reading request/response body */
	ST_READING_TRAILER,	/**< reading request/response chunked trailer */
	ST_WRITING		/**< writing request/response headers/body */
};

class HttpChannelLibEvent {
public:
  HttpChannelLibEvent(ChannelInfo info, event_base* base);
  ~HttpChannelLibEvent();

  void InitConnection();
  void CloseConnection();

  void OnChannelClosed();

  uint32_t TimeoutMs() {return timeout_ms_; }
  void SetTimeoutMs(uint32_t ms) {timeout_ms_ = ms;}

  ChannelStatus Status();
  evhttp_connection* EvHttpConnection() {
    return connection_;
  }
private:
  event_base* ev_base_;
  struct evdns_base* evdns_base_;
  struct evhttp_connection* connection_;

  ChannelInfo channel_info_;
  uint32_t timeout_ms_;
};

}// end namespace net
#endif
