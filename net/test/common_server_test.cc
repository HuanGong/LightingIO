#include "../server.h"
#include "../tcp_channel.h"

#include <functional>
#include <glog/logging.h>
#include "../protocol/proto_service.h"
#include "../protocol/line/line_message.h"
#include "../protocol/raw/raw_message.h"
#include "../protocol/http/http_request.h"
#include "../protocol/http/http_response.h"
#include "../protocol/http/http_proto_service.h"

namespace net {

class Application: public SrvDelegate {
public:
  ~Application() {
  }
};
}//end namespace

void handler(net::RefProtocolMessage message) {
  CHECK(message.get());

  net::LineMessage* linemsg = static_cast<net::LineMessage*>(message.get());
  DLOG(INFO) << "I Got LineMessage: body:\n" << linemsg->Body();
  std::shared_ptr<net::LineMessage> res =
    std::make_shared<net::LineMessage>(net::IODirectionType::kOutResponse);
  res->MutableBody() = "Hello World\n";
  linemsg->SetResponse(std::move(res));
}

void http_handler(net::RefProtocolMessage message) {
  net::HttpRequest* httpmsg = static_cast<net::HttpRequest*>(message.get());
  net::IOBuffer buffer;
  net::HttpProtoService::RequestToBuffer(httpmsg, &buffer);
  LOG(INFO) << "I Got HttpRequest Raw:\n" << buffer.AsString();

  net::RefHttpResponse response =
    std::make_shared<net::HttpResponse>(net::IODirectionType::kOutResponse);
  response->SetResponseCode(200);
  response->SetKeepAlive(httpmsg->IsKeepAlive());
  response->MutableBody() = "Nice to meet your,I'm LightingIO\n";
  httpmsg->SetResponse(std::move(response));
}

void raw_handler(net::RefProtocolMessage message) {
  net::RawMessage* request = static_cast<net::RawMessage*>(message.get());
  LOG(INFO) << "I Got A RawRequest:\n" << request->MessageDebug();
}

int main(int argc, char* argv[]) {
  //google::InitGoogleLogging(argv[0]);  // 初始化 glog
  google::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags
  base::MessageLoop2 main_loop;
  main_loop.SetLoopName("MainLoop");
  main_loop.Start();

  net::Application app;
  net::Server server((net::SrvDelegate*)&app);

  //server.RegisterService("http://0.0.0.0:5006",
  //                       std::bind(http_handler, std::placeholders::_1));

  //server.RegisterService("line://0.0.0.0:5001",
  //                       std::bind(handler, std::placeholders::_1));

  server.RegisterService("raw://0.0.0.0:5002",
                         std::bind(raw_handler, std::placeholders::_1));

  server.RunAllService();

  main_loop.WaitLoopEnd();
}

