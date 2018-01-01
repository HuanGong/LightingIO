#ifndef _NET_CALLBACK_DEF_H_H
#define _NET_CALLBACK_DEF_H_H

#include <string>
#include <memory>
#include <functional>

namespace net {

class IOBuffer;
class InetAddress;
class ServiceAcceptor;
class TcpChannel;
class IOService;
class ProtoService;
class ProtocolMessage;

/* ============ connection channel relative =========*/
typedef std::shared_ptr<TcpChannel> RefTcpChannel;
typedef std::weak_ptr<TcpChannel> WeakPtrTcpChannel;

typedef std::function<void(const RefTcpChannel&)> FinishSendCallback;
typedef std::function<void(const RefTcpChannel&)> ChannelClosedCallback;
typedef std::function<void(const RefTcpChannel&)> ChannelStatusCallback;
typedef std::function<void(const RefTcpChannel&, IOBuffer*)> RcvDataCallback;


/* ============= Service Acceptor relative ===========*/
typedef std::shared_ptr<ServiceAcceptor> RefServiceAcceptor;
typedef std::function<void(int/*socket_fd*/, const InetAddress&)> NewConnectionCallback;

/* protoservice handle tcpmessage to different type protocol request and message */
typedef std::shared_ptr<ProtoService> RefProtoService;

/* ============== io service ====== */
typedef std::shared_ptr<IOService> RefIOService;
//typedef std::function<void
//std::map<std::string/*protocol*/,
typedef std::shared_ptr<ProtocolMessage> RefProtocolMessage;
typedef std::unique_ptr<ProtocolMessage> OwnedProtocolMessage;

typedef std::function<void(const RefProtocolMessage/*request*/)> ProtoMessageHandler;

}
#endif
