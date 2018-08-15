

#include "io_service.h"
#include "glog/logging.h"
#include "tcp_channel.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service_factory.h"

#include <atomic>
#include <base/base_constants.h>
#include <base/coroutine/coroutine_scheduler.h>

namespace net {

IOService::IOService(const InetAddress addr,
                     const std::string protocol,
                     base::MessageLoop* workloop,
                     IOServiceDelegate* delegate)
  : protocol_(protocol),
    acceptor_loop_(workloop),
    delegate_(delegate),
    dispatcher_(NULL),
    is_stopping_(false) {

  CHECK(delegate_);

  service_name_ = addr.IpPortAsString();
  acceptor_.reset(new ServiceAcceptor(acceptor_loop_->Pump(), addr));
  acceptor_->SetNewConnectionCallback(std::bind(&IOService::OnNewConnection,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

IOService::~IOService() {
  LOG(INFO) << "IOServce protocol " << protocol_ << " Gone";
}

void IOService::StartIOService() {

  if (!acceptor_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::StartIOService, this);
    acceptor_loop_->PostTask(base::NewClosure(std::move(functor)));
    return;
  }

  acceptor_->StartListen();
  delegate_->IOServiceStarted(this);
}

/* step1: close the acceptor */
void IOService::StopIOService() {
  if (!acceptor_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::StopIOService, this);
    acceptor_loop_->PostTask(base::NewClosure(std::move(functor)));
    return;
  }

  //sync
  acceptor_->StopListen();

  is_stopping_ = true;

  //async
  for (auto& channel : connections_) {
    base::MessageLoop* loop = channel->IOLoop();
    //loop->PostTask(base::NewClosure(std::bind(&TcpChannel::ShutdownChannel, channel)));
    loop->PostTask(base::NewClosure(std::bind(&TcpChannel::ForceShutdown, channel)));
  }

  if (connections_.size() == 0) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::OnNewConnection(int local_socket, const InetAddress& peer_addr) {
  CHECK(acceptor_loop_->IsInLoopThread());

  //max connction reached
  if (!delegate_->CanCreateNewChannel()) {
    socketutils::CloseSocket(local_socket);
    LOG(INFO) << "Max Channels Limit, Current Has:[" << channel_count_ << "] Channels";
    return;
  }


  base::MessageLoop* io_work_loop = delegate_->GetNextIOWorkLoop();
  if (!io_work_loop) {
    socketutils::CloseSocket(local_socket);
    return;
  }

  RefProtoService proto_service = ProtoServiceFactory::Create(protocol_);
  if (!proto_service || !message_handler_) {
    LOG(ERROR) << "no proto parser or no message handler, close this connection.";
    socketutils::CloseSocket(local_socket);
    return;
  }
  proto_service->SetMessageHandler(message_handler_);
  proto_service->SetServiceType(ProtocolServiceType::kServer);

  net::InetAddress local_addr(socketutils::GetLocalAddrIn(local_socket));

  auto new_channel = TcpChannel::CreateServerChannel(local_socket,
                                                     local_addr,
                                                     peer_addr,
                                                     io_work_loop);

  //TODO: Is peer_addr.IpPortAsString Is unique?
  const std::string channel_name = peer_addr.IpPortAsString();
  new_channel->SetChannelName(channel_name);
  new_channel->SetOwnerLoop(acceptor_loop_);
  new_channel->SetProtoService(proto_service);
  new_channel->SetCloseCallback(std::bind(&IOService::OnChannelClosed, this, std::placeholders::_1));

  new_channel->Start();

  VLOG(GLOG_VTRACE) << " New Connection from:" << channel_name;

  StoreConnection(new_channel);
}

void IOService::OnChannelClosed(const RefTcpChannel& connection) {
  if (!acceptor_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::RemoveConncetion, this, connection);
    acceptor_loop_->PostTask(base::NewClosure(std::move(functor)));
    return;
  }
  RemoveConncetion(connection);
}

void IOService::StoreConnection(const RefTcpChannel connection) {
  CHECK(acceptor_loop_->IsInLoopThread());

  connections_.insert(connection);

  channel_count_.store(connections_.size());

  delegate_->IncreaseChannelCount();
}

void IOService::RemoveConncetion(const RefTcpChannel connection) {
  CHECK(acceptor_loop_->IsInLoopThread());

  connections_.erase(connection);

  channel_count_.store(connections_.size());

  delegate_->DecreaseChannelCount();

  if (is_stopping_ && connections_.size() == 0) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::SetProtoMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

void IOService::SetWorkLoadDispatcher(WorkLoadDispatcher* d) {
  dispatcher_ = d;
}

}// endnamespace net
