#ifndef NET_PROTOCOL_MESSAGE_H
#define NET_PROTOCOL_MESSAGE_H

#include <string>
#include <net_callback.h>
#include <base/coroutine/coroutine.h>
#include <base/event_loop/msg_event_loop.h>

namespace net {

typedef enum {
  kUndefined = 0,
  kInRequest = 1,
  kInReponse = 2,
  kOutRequest = 3,
  kOutResponse = 4,
  kUnknownType = 5
} IODirectionType;

typedef struct {
  WeakPtrTcpChannel channel;
} IOContext;

typedef enum {
  kNothing = 0,
  kTimeOut = 1,
  kChannelBroken = 2
}FailInfo;

typedef struct {
  void* data;
  base::MessageLoop2* coro_loop;
  std::weak_ptr<base::Coroutine> weak_coro;
} WorkContext;

class ProtocolMessage {
public:
  ProtocolMessage(IODirectionType, const std::string);
  virtual ~ProtocolMessage();

  const std::string& Protocol() const;

  void SetMessageDirection(IODirectionType t);
  const IODirectionType MessageDirection() const;

  IOContext& GetIOCtx() {return io_context_;}
  WorkContext& GetWorkCtx() {return work_context_;}
  void SetIOContextWeakChannel(const RefTcpChannel& channel);
  void SetIOContextWeakChannel(WeakPtrTcpChannel& channel);

  void SetFailInfo(FailInfo reason);
  FailInfo MessageFailInfo() const;

  void SetResponse(RefProtocolMessage& response);
  void SetResponse(RefProtocolMessage&& response);
  RefProtocolMessage Response() {return response_;}

  virtual const std::string MessageDebug() {};
protected:
  IOContext io_context_;
  WorkContext work_context_;
private:
  // Work Context
  FailInfo fail_info_;
  std::string proto_;
  IODirectionType direction_;
  RefProtocolMessage response_;
};

}
#endif
