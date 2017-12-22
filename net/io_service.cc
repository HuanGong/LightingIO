

#include "io_service.h"
#include "glog/logging.h"
#include "proto_service.h"
#include "tcp_channel.h"

namespace net {

IOService::IOService(const InetAddress addr,
                     const std::string protocol,
                     base::MessageLoop2* workloop,
                     IOServiceDelegate* delegate)
  : as_dispatcher_(true),
    protocol_(protocol),
    work_loop_(workloop),
    delegate_(delegate) {

  CHECK(delegate_);

  acceptor_.reset(new ServiceAcceptor(work_loop_, addr));

  acceptor_->SetNewConnectionCallback(
    std::bind(&IOService::HandleNewConnection, this, std::placeholders::_1, std::placeholders::_2));

  proto_service_ = delegate_->GetProtocolService(protocol_);
}

IOService::~IOService() {

}

void IOService::StartIOService() {
  if (work_loop_->IsInLoopThread()) {
    acceptor_->StartListen();
  } else {
    auto functor = std::bind(&ServiceAcceptor::StartListen, acceptor_);
    work_loop_->PostTask(base::NewClosure(std::move(functor)));
  }
}

void IOService::StopIOService() {

}

void IOService::HandleNewConnection(int local_socket, const InetAddress& peer_addr) {
  if (!delegate_) {
    LOG(ERROR) << "New Connection Can't Get IOWorkLoop From NULL delegate";
    socketutils::CloseSocket(local_socket);
    return;
  }
  //max connction reached
  if (!delegate_->CanCreateNewChannel()) {
    socketutils::CloseSocket(local_socket);
    LOG(INFO) << "Max Channels Reached, Current IOservice Has:[" << channel_count_ << "] Channel";
    return;
  }

  base::MessageLoop2* io_work_loop = delegate_->GetNextIOWorkLoop();
  LOG(INFO) << " New Connection from:" << peer_addr.IpPortAsString();

  net::InetAddress local_addr(socketutils::GetLocalAddrIn(local_socket));
  auto new_channel = TcpChannel::Create(local_socket,
                                               local_addr,
                                               peer_addr,
                                               io_work_loop);
  new_channel->SetChannelName("test");
  new_channel->SetOwnerLoop(work_loop_);
  new_channel->SetCloseCallback(std::bind(&IOService::OnChannelClosed,
                                          this,
                                          std::placeholders::_1));
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

  /* for uniform logic just post task to owner handle this*/
  //if (work_loop_->IsInLoopThread()) {
    //StoreConnection(f);
  //}
  work_loop_->PostTask(base::NewClosure(
      std::bind(&IOService::StoreConnection, this, new_channel)));

  //f->ShutdownConnection();
}

void IOService::OnChannelClosed(const RefTcpChannel& connection) {
  if (work_loop_->IsInLoopThread()) {
    RemoveConncetion(connection);
  } else {
    work_loop_->PostTask(base::NewClosure(
        std::bind(&IOService::RemoveConncetion, this, connection)));
  }
}


void IOService::StoreConnection(const RefTcpChannel connection) {
  CHECK(work_loop_->IsInLoopThread());
  connections_[connection->ChannelName()] = connection;

  channel_count_.store(connections_.size());

  if (delegate_) {
    delegate_->IncreaseChannelCount();
  }
}

void IOService::RemoveConncetion(const RefTcpChannel connection) {
  CHECK(work_loop_->IsInLoopThread());
  connections_.erase(connection->ChannelName());

  channel_count_.store(connections_.size());

  if (delegate_) {
    delegate_->DecreaseChannelCount();
  }
}

}// endnamespace net
