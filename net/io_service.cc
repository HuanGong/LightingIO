

#include "io_service.h"
#include "glog/logging.h"
#include "tcp_channel.h"
#include "protocol/proto_service.h"
#include "protocol/proto_service_factory.h"

namespace net {

IOService::IOService(const InetAddress addr,
                     const std::string protocol,
                     base::MessageLoop2* workloop,
                     IOServiceDelegate* delegate)
  : protocol_(protocol),
    acceptor_loop_(workloop),
    delegate_(delegate),
    is_stopping_(false) {

  CHECK(delegate_);
  service_name_ = addr.IpPortAsString();
  acceptor_.reset(new ServiceAcceptor(acceptor_loop_->Pump(), addr));
  acceptor_->SetNewConnectionCallback(std::bind(&IOService::HandleNewConnection,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

IOService::~IOService() {

}

void IOService::StartIOService() {
  if (!acceptor_loop_->IsInLoopThread()) {
    auto functor = std::bind(&ServiceAcceptor::StartListen, acceptor_);
    acceptor_loop_->PostTask(base::NewClosure(std::move(functor)));
    return;
  }

  acceptor_->StartListen();
  delegate_->IOServiceStarted(this);
}

/* step1: close the acceptor */
void IOService::StopIOService() {
  if (!acceptor_loop_->IsInLoopThread()) {
    auto functor = std::bind(&ServiceAcceptor::StopListen, acceptor_);
    acceptor_loop_->PostTask(base::NewClosure(std::move(functor)));
    return;
  }

  //sync
  acceptor_->StopListen();

  is_stopping_ = true;

  //async
  for (auto& pair : connections_) {
    pair.second->ForceShutdown();
  }

  if (connections_.size() == 0) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::HandleNewConnection(int local_socket, const InetAddress& peer_addr) {
  CHECK(acceptor_loop_->IsInLoopThread());
  if (!delegate_) {
    LOG(ERROR) << "New Connection Can't Get IOWorkLoop From NULL delegate";
    socketutils::CloseSocket(local_socket);
    return;
  }
  //max connction reached
  if (!delegate_->CanCreateNewChannel()) {
    socketutils::CloseSocket(local_socket);
    LOG(INFO) << "Max Channels Limit, Current Has:[" << channel_count_ << "] Channels";
    return;
  }

  base::MessageLoop2* io_work_loop = delegate_->GetNextIOWorkLoop();
  if (!io_work_loop) {
    socketutils::CloseSocket(local_socket);
    return;
  }
  LOG(INFO) << " New Connection from:" << peer_addr.IpPortAsString();

  net::InetAddress local_addr(socketutils::GetLocalAddrIn(local_socket));
  auto new_channel = TcpChannel::Create(local_socket,
                                        local_addr,
                                        peer_addr,
                                        io_work_loop);

  //Is peer_addr.IpPortAsString Is unique?
  new_channel->SetChannelName(peer_addr.IpPortAsString());

  new_channel->SetOwnerLoop(acceptor_loop_);
  new_channel->SetCloseCallback(std::bind(&IOService::OnChannelClosed,
                                          this,
                                          std::placeholders::_1));

  RefProtoService proto_service =
    ProtoServiceFactory::Instance().Create(protocol_);

  proto_service->SetMessageDispatcher(delegate_->MessageDispatcher());

  proto_service->SetMessageHandler(message_handler_);

  new_channel->SetProtoService(proto_service);

  /*
  RcvDataCallback data_cb = std::bind(&ProtoService::OnDataRecieved,
                                      proto_service_,
                                      std::placeholders::_1,
                                      std::placeholders::_2);
  new_channel->SetDataHandleCallback(std::move(data_cb));

  ChannelStatusCallback status_cb = std::bind(&ProtoService::OnStatusChanged,
                                              proto_service_,
                                              std::placeholders::_1);
  new_channel->SetStatusChangedCallback(std::move(status_cb));

  FinishSendCallback finish_writen = std::bind(&ProtoService::OnDataFinishSend,
                                               proto_service_,
                                               std::placeholders::_1);
  new_channel->SetFinishSendCallback(std::move(finish_writen));
  */

  StoreConnection(new_channel);
}

void IOService::OnChannelClosed(const RefTcpChannel& connection) {
  if (!acceptor_loop_->IsInLoopThread()) {
    acceptor_loop_->PostTask(base::NewClosure(
        std::bind(&IOService::RemoveConncetion, this, connection)));
    return;
  }
  RemoveConncetion(connection);
}

void IOService::StoreConnection(const RefTcpChannel connection) {
  CHECK(acceptor_loop_->IsInLoopThread());
  connections_[connection->ChannelName()] = connection;

  channel_count_.store(connections_.size());

  if (delegate_) {
    delegate_->IncreaseChannelCount();
  }
}

void IOService::RemoveConncetion(const RefTcpChannel connection) {
  CHECK(acceptor_loop_->IsInLoopThread());
  connections_.erase(connection->ChannelName());

  channel_count_.store(connections_.size());

  if (delegate_) {
    delegate_->DecreaseChannelCount();
    if (is_stopping_) {
      delegate_->IOServiceStoped(this);
    }
  }
}

void IOService::SetProtoMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

}// endnamespace net
