#ifndef _NET_CALLBACK_DEF_H_H
#define _NET_CALLBACK_DEF_H_H

#include <string>
#include <memory>
#include <functional>

namespace net {

class IOBuffer;
class InetAddress;
class ServiceAcceptor;
class ConnectionChannel;

class ProtoService;

/* ============ connection channel relative =========*/
typedef std::shared_ptr<ConnectionChannel> RefConnectionChannel;

typedef std::function<void(const RefConnectionChannel&)> DataWritenCallback;
typedef std::function<void(const RefConnectionChannel&, IOBuffer*)> DataRcvCallback;
typedef std::function<void(const RefConnectionChannel&)> ConnectionClosedCallback;
typedef std::function<void(const RefConnectionChannel&)> ConnectionStatusCallback;

/* ============= Service Acceptor relative ===========*/
typedef std::shared_ptr<ServiceAcceptor> RefServiceAcceptor;
typedef std::function<void(int/*socket_fd*/, const InetAddress&)> NewConnectionCallback;

/* protoservice handle tcpmessage to different type protocol request and message */
typedef std::shared_ptr<ProtoService> RefProtoService;

/* ============== io service ====== */
//typedef std::function<void
//std::map<std::string/*protocol*/,

}
#endif
