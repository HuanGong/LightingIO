#include <glog/logging.h>

#include <vector>
#include "../tcp_channel.h"
#include "../socket_utils.h"
#include "../service_acceptor.h"
#include "../protocol/proto_service.h"
#include "../protocol/line/line_message.h"
#include "../protocol/http/http_request.h"
#include "../protocol/http/http_response.h"
#include "../protocol/proto_service_factory.h"
#include "../inet_address.h"
#include "clients/client_connector.h"
#include "dispatcher/coro_dispatcher.h"

base::MessageLoop2 loop;
net::InetAddress server_address("0.0.0.0", 5006);
net::CoroWlDispatcher* g_dispatcher = NULL;

namespace net {

class Client : public ConnectorDelegate {
public:
  Client() {
    connector_ = std::make_shared<Connector>(&loop, this);
    message_handler_ = std::bind(&Client::OnResponseMessage, this, std::placeholders::_1);
  }
  ~Client() {
  }

  void Start() {
    connector_->LaunchAConnection(server_address);
  }

  void OnNewClientConnected(int socket_fd, InetAddress& local, InetAddress& remote) override {

    LOG(ERROR) << " OnNewClientConnected ";
    RefTcpChannel new_channel = TcpChannel::CreateClientChannel(socket_fd,
                                                                local,
                                                                remote,
                                                                &loop);
    //Is peer_addr.IpPortAsString Is unique?
    new_channel->SetChannelName(remote.IpPortAsString());
    new_channel->SetOwnerLoop(&loop);

    new_channel->SetCloseCallback(std::bind(&Client::OnChannelClosed,
                                            this,
                                            std::placeholders::_1));

    RefProtoService proto_service = ProtoServiceFactory::Instance().Create("http");
    proto_service->SetMessageDispatcher(g_dispatcher);

    proto_service->SetMessageHandler(message_handler_);
    new_channel->SetProtoService(proto_service);

    channels_.push_back(new_channel);
  }

  RefProtocolMessage SendRequest(RefProtocolMessage& request) {
  }
private:
  void OnResponseMessage(const RefProtocolMessage message) {
    LOG(ERROR) << "OnResponseMessage";
  }

  void OnChannelClosed(RefTcpChannel channel) {
    CHECK(loop.IsInLoopThread());
    auto iter = std::find(channels_.begin(), channels_.end(), channel);
    if (iter != channels_.end()) {
      channels_.erase(iter);
    } else {
      LOG(ERROR) << "Not Should Happend";
    }
  }
  ProtoMessageHandler message_handler_;
  std::vector<RefTcpChannel> channels_;
  std::shared_ptr<Connector> connector_;
};

}//

net::RefHttpRequest g_request;
net::RefTcpChannel g_channel;

int main(int argc, char* argv[]) {

  g_dispatcher = new net::CoroWlDispatcher(true);
  loop.SetLoopName("clientloop");
  loop.Start();

  std::shared_ptr<net::Client> client = std::make_shared<net::Client>();
  loop.PostTask(base::NewClosure(std::bind(&net::Client::Start, client)));

  loop.WaitLoopEnd();
  return 0;
}
