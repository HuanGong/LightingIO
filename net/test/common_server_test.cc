#include "../server.h"
#include <glog/logging.h>
#include "../protocol/proto_service.h"
#include "../tcp_channel.h"
#include <functional>

namespace net {

class TcpProtoService : public ProtoService {
public:
  TcpProtoService() :
    ProtoService("tcp") {
  }
  ~TcpProtoService() {}

  void SetMessageHandler(ProtoMessageHandler handler) {
    message_handler_ = handler;
  }
  void OnStatusChanged(const RefTcpChannel& channel) override {
    LOG(INFO) << "RefTcpChannel status changed:" << channel->StatusAsString();
  }
  void OnDataFinishSend(const RefTcpChannel& channel) override {
    LOG(INFO) << " RefTcpChannel  data write finished";
  }
  void OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buffer) override {
    LOG(INFO) << " RefTcpChannel  recieve data:" << buffer->CanReadSize() << " bytes";
    int32_t size = channel->Send(buffer->GetRead(), buffer->CanReadSize());
    if (size > 0) {
      buffer->Consume(size);
    }

    //channel->ShutdownChannel();
  }
private:
  ProtoMessageHandler message_handler_;
};

class Application: public SrvDelegate {
public:
  ~Application() {
    tcp_.reset();
  }
  void RegisterProtoService(ProtoserviceMap& map) override {
    LOG(INFO) << __FUNCTION__ << " enter";
    tcp_.reset(new TcpProtoService());
    map["tcp"] = tcp_;
  }
private:
  std::shared_ptr<ProtoService> tcp_;
};

}//end namespace

void handler(net::RefProtocolMessage message) {

}

int main() {

  //net::SrvDelegate delegate;
  net::Application app;
  net::Server server((net::SrvDelegate*)&app);

  server.RegisterService("tcp://0.0.0.0:5005",
                         std::bind(handler, std::placeholders::_1));
  server.RunAllService();

  LOG(INFO) << __FUNCTION__ << " program end";
}
