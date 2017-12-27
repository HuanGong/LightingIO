
#include "../protocol/proto_service.h"
#include "glog/logging.h"
#include "../tcp_channel.h"
#include "../io_service.h"
#include "base/event_loop/linux_signal.h"

namespace net {

class TcpProtoService : public ProtoService {
public:
  TcpProtoService() :
    ProtoService("tcp") {
  }
  ~TcpProtoService() {
  }

  virtual void SetMessageHandler(ProtoMessageHandler) {
    ;
  }

  void OnStatusChanged(const RefTcpChannel& channel) override {
    LOG(INFO) << "RefTcpChannel status changed:" << channel->StatusAsString();
  }
  void OnDataFinishSend(const RefTcpChannel& channel) override {
    LOG(INFO) << " RefTcpChannel  data write finished";
  }

  void OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buffer) override {
    LOG(INFO) << " RefTcpChannel  recieve data";

    int32_t size = channel->Send(buffer->GetRead(),
                                 buffer->CanReadSize());
    if (size > 0) {
      buffer->Consume(size);
    }
    channel->ShutdownChannel();
  }
};

class Srv : public IOServiceDelegate {
public:
  Srv() {
    tcp_protoservice_.reset(new TcpProtoService);
    InitWorkLoop();

    net::InetAddress addr(5005);
    ioservice_.reset(new IOService(addr,
                                   "tcp",
                                   &acceptor_loop_,
                                   this));
  }
  ~Srv() {
  }


  void InitWorkLoop() {
    iowork_loop_.SetLoopName("iowork_loop");
    iowork_loop_.Start();
    acceptor_loop_.SetLoopName("acceptor_loop");
    acceptor_loop_.Start();
  }

  void Start() {
    RegisterExitSignal();

    ioservice_->StartIOService();

    acceptor_loop_.WaitLoopEnd();
  }

  void IOServiceStoped(IOService* ioservice) {
    LOG(INFO) << " IOService stop ok";
    CHECK(ioservice_.get() == ioservice);
    ioservice_.reset();
    acceptor_loop_.QuitLoop();
  };

  base::MessageLoop2* GetNextIOWorkLoop() {
    return &iowork_loop_;
  }
  bool IncreaseChannelCount() {
    LOG(INFO) << " a new channel comming";
  }
  void DecreaseChannelCount() {
    LOG(INFO) << " a channel going gone";
  }
  bool CanCreateNewChannel() {
    return true;
  }
  RefProtoService GetProtocolService(const std::string protocol) {
    return tcp_protoservice_;
  }
private:
  void RegisterExitSignal() {
    //CHECK(acceptor_loop_.IsInLoopThread());
    base::Signal::signal(10, std::bind(&Srv::ExitSignalHandle, this));
  }
  void ExitSignalHandle() {
    LOG(INFO) << " ExitSignalHandle quit AcceptorLoop";
    acceptor_loop_.PostTask(
      base::NewClosure(std::bind(&IOService::StopIOService, ioservice_)));
  }
  base::MessageLoop2 iowork_loop_;
  base::MessageLoop2 acceptor_loop_;

  RefIOService ioservice_;
  RefProtoService tcp_protoservice_;
};

}

int main() {
  net::Srv s;
  s.Start();
  //LOG(INFO) << " main is going to end";
  return 0;
}
