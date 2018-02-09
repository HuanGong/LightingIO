

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
                     base::MessageLoop2* workloop,
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
  for (auto& channel : connections_) {
    channel->ForceShutdown();
  }

  if (connections_.size() == 0) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::OnNewConnection(int local_socket, const InetAddress& peer_addr) {
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
  VLOG(GLOG_VTRACE) << " New Connection from:" << peer_addr.IpPortAsString();

  net::InetAddress local_addr(socketutils::GetLocalAddrIn(local_socket));
  auto new_channel = TcpChannel::Create(local_socket,
                                        local_addr,
                                        peer_addr,
                                        io_work_loop,
                                        ChannelServeType::kServerType);

  //Is peer_addr.IpPortAsString Is unique?
  new_channel->SetChannelName(peer_addr.IpPortAsString());

  new_channel->SetOwnerLoop(acceptor_loop_);
  new_channel->SetCloseCallback(std::bind(&IOService::OnChannelClosed,
                                          this,
                                          std::placeholders::_1));

  RefProtoService proto_service =
    ProtoServiceFactory::Instance().Create(protocol_);

  proto_service->SetMessageDispatcher(delegate_->MessageDispatcher());

  ProtoMessageHandler h = std::bind(&IOService::HandleRequest, this, std::placeholders::_1);
  proto_service->SetMessageHandler(h);

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

  connections_.insert(connection);
  //connections_[connection->ChannelName()] = connection;

  channel_count_.store(connections_.size());

  if (delegate_) {
    delegate_->IncreaseChannelCount();
  }
}

void IOService::RemoveConncetion(const RefTcpChannel connection) {
  CHECK(acceptor_loop_->IsInLoopThread());

  connections_.erase(connection);

  channel_count_.store(connections_.size());

  if (delegate_) {
    delegate_->DecreaseChannelCount();

    if (is_stopping_ && connections_.size() == 0) {
      delegate_->IOServiceStoped(this);
    }
  }
}

//Running On NetWorkIO Channel Thread
void IOService::HandleRequest(const RefProtocolMessage& request) {
  //LOG(INFO) << "Server [" << service_name_ << "] Recv a Request";
  //current just Handle Work On IO
  CHECK(request);

  WeakPtrTcpChannel weak_channel = request->GetIOCtx().channel;
  RefTcpChannel channel = weak_channel.lock();
  if (!channel) {
    return;
  }

  CHECK(channel->InIOLoop());

  if (!message_handler_) {//replay default response some type request no response
    channel->ShutdownChannel();
    return;
  }

  base::StlClourse functor = std::bind(&IOService::HandleRequestOnWorker, this, request);

  bool ok = dispatcher_->TransmitToWorker(functor);
  if (false == ok) {
    channel->ShutdownChannel();
  }
}

static std::atomic<int> counter;
//Run On Worker
void IOService::HandleRequestOnWorker(const RefProtocolMessage request) {

  do {
    if (!dispatcher_->SetWorkContext(request.get())) {
      break;
    }
    if (message_handler_) {
      message_handler_(request);
    }
  } while(0);

  WeakPtrTcpChannel weak_channel = request->GetIOCtx().channel;
  RefTcpChannel channel = weak_channel.lock();
  if (NULL == channel.get() || false == channel->IsConnected()) {
    VLOG(GLOG_VTRACE) << __FUNCTION__ << " Channel Has Broken After Handle Request Message";
    return;
  }

  RefProtocolMessage response = request->Response();
  if (!response.get()) {
    response = channel->GetProtoService()->DefaultResponse(request);
  }
  if (!response) { //This type message not need response, WorkFlow Over
    return;
  }

  bool close = channel->GetProtoService()->CloseAfterMessage(request.get(), response.get());
  LOG_IF(ERROR, close == false) << "This Connection KeepAlive " << (counter++);

  if (channel->InIOLoop()) { //send reply directly
    bool send_success = channel->SendProtoMessage(response);
    if (close || !send_success) { //failed
      channel->ShutdownChannel();
    }
  } else  { //Send Response to Channel's IO Loop
    auto functor = [=]() {
      bool send_success = channel->SendProtoMessage(response);
      if (close || !send_success) { //failed
        channel->ShutdownChannel();
      }
    };
    channel->IOLoop()->PostTask(base::NewClosure(std::move(functor)));
  }
}

void IOService::SetProtoMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

void IOService::SetWorkLoadDispatcher(WorkLoadDispatcher* d) {
  dispatcher_ = d;
}

}// endnamespace net
