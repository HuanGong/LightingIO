#ifndef SOCKET_CLIENT_CHANNEL_
#define SOCKET_CLIENT_CHANNEL_

#include <string>

namespace net {

struct ChannelInfo {
  ChannelInfo(): port(-1) {
  }
  int port;
  std::string host;
};

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

class ClientChannel {
public:
  ClientChannel(ChannelInfo channel_info);
  virtual ~ClientChannel();

  virtual void Init() = 0;
  virtual void Close() = 0;

  void SetTimeout(uint32_t timeout_ms);
  void SetAutoReconnection(bool auto_reconnect);
  void SetReconnectioninterval(uint32_t interval);

  int ChannelPort() {return channel_info_.port; }
  std::string ChannelIp() { return channel_info_.host; }
  uint32_t ChannelTimeoutMs() { return timeout_ms_; }
  ChannelStatus ConnectionStatus();
private:
  uint32_t timeout_ms_;
  bool auto_reconnection_;
  uint32_t reconnection_interval_;

  ChannelStatus status_;

  ChannelInfo channel_info_;
};

} //end namesapce
#endif
