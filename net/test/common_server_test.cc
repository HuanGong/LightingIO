#include "../server.h"
#include <glog/logging.h>
#include "../protocol/proto_service.h"
#include "../tcp_channel.h"
#include <functional>
#include "../protocol/line/line_message.h"
#include "../protocol/http/http_request.h"
#include "../protocol/http/http_response.h"

namespace net {

class Application: public SrvDelegate {
public:
  ~Application() {
  }
private:
};
}//end namespace

void handler(net::RefProtocolMessage message) {
  CHECK(message.get());

  net::LineMessage* linemsg = static_cast<net::LineMessage*>(message.get());
  LOG(INFO) << "I Got LineMessage: body:" << linemsg->Body();
  std::shared_ptr<net::LineMessage> res =
    std::make_shared<net::LineMessage>(net::IODirectionType::kOutResponse);
  res->MutableBody() = "hello response";
  linemsg->SetResponse(std::move(res));
}

void http_handler(net::RefProtocolMessage message) {
  net::HttpRequest* httpmsg = static_cast<net::HttpRequest*>(message.get());
  std::ostringstream oss;
  httpmsg->ToRequestRawData(oss);

  LOG(INFO) << "I Got HttpRequest:" << httpmsg->MessageDebug() << " \n Raw: \n" << oss.str();
  net::RefHttpResponse response = std::make_shared<net::HttpResponse>(net::IODirectionType::kOutResponse);
  response->SetResponseCode(404);
  response->SetKeepAlive(false);
  response->MutableBody() = "Nice to meet your,I'm LightingIO\n";

  httpmsg->SetResponse(std::move(response));
}

int main() {

  //net::SrvDelegate delegate;
  net::Application app;
  net::Server server((net::SrvDelegate*)&app);

  //server.RegisterService("line://0.0.0.0:5005",
  //                       std::bind(handler, std::placeholders::_1));
  server.RegisterService("http://0.0.0.0:5006",
                         std::bind(http_handler, std::placeholders::_1));
  server.RunAllService();

  LOG(INFO) << __FUNCTION__ << " program end";
}
