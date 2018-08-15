#ifndef NET_PROTOCOL_MESSAGE_H
#define NET_PROTOCOL_MESSAGE_H

#include <string>
#include <net_callback.h>
#include <base/coroutine/coroutine.h>
#include <base/message_loop/message_loop.h>

namespace net {

typedef enum {
  kNone      = 0x0,
  kRequest   = 0x1,
  kResponse  = 0x2,
} MessageType;

typedef enum {
  kNothing = 0,
  kTimeOut = 1,
  kChannelBroken = 2
} FailInfo;

typedef struct {
  WeakPtrTcpChannel channel;
} IOContext;

typedef struct {
  uint64_t task_identify;
  base::MessageLoop* loop;
  std::weak_ptr<base::Coroutine> weak_coro;
} WorkContext;

class ProtocolMessage;
typedef std::shared_ptr<ProtocolMessage> RefProtocolMessage;
typedef std::function<void(const RefProtocolMessage&/*request*/)> ProtoMessageHandler;

class ProtocolMessage {
public:
  ProtocolMessage(const std::string);
  virtual ~ProtocolMessage();

  const std::string& Protocol() const;

  void SetMessageType(MessageType t) {type_ = t;};
  const MessageType ProtocolMessageType() const {return type_;};

  IOContext& GetIOCtx() {return io_context_;}
  WorkContext& GetWorkCtx() {return work_context_;}
  void SetIOContextWeakChannel(const RefTcpChannel& channel);
  void SetIOContextWeakChannel(WeakPtrTcpChannel& channel);

  void SetFailInfo(FailInfo reason);
  FailInfo MessageFailInfo() const;
  const std::string& FailMessage() const;
  void AppendFailMessage(std::string m);

  void SetResponse(RefProtocolMessage& response);
  void SetResponse(RefProtocolMessage&& response);
  RefProtocolMessage& Response() {return response_;}

  const char* MessageTypeStr() const;
  virtual const std::string MessageDebug() const {return "";};
protected:
  IOContext io_context_;
  WorkContext work_context_;
private:
  // Work Context
  FailInfo fail_info_;
  std::string fail_;
  std::string proto_;
  MessageType type_;
  RefProtocolMessage response_;
};

}
#endif
