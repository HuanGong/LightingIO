#include "../service_acceptor.h"

#include "../net_callback.h"
#include "../tcp_channel.h"
#include "../socket_utils.h"

#include <functional>
#include <glog/logging.h>
#include <net/io_buffer.h>
#include <net/protocol/http/http_request.h>
#include <net/protocol/http/http_response.h>
#include <net/protocol/http/http_proto_service.h>
#include <sstream>


int main(int argc, char** argv) {

  net::RefHttpRequest request = std::make_shared<net::HttpRequest>();
  request->SetMethod("GET");
  request->MutableBody() = "Nice to Meet you!";
  std::string field("Name");
  std::string value("HuanGong");
  request->InsertHeader(field, value);
  request->SetRequestURL("/abc");
  net::IOBuffer req_buffer;
  net::HttpProtoService::RequestToBuffer(request.get(), &req_buffer);
  LOG(ERROR) << "request:\n"  << req_buffer.AsString();

  net::RefHttpResponse response = std::make_shared<net::HttpResponse>();
  response->SetResponseCode(200);
  response->SetKeepAlive(true);
  response->MutableBody() = "Nice to meet your,I'm LightingIO";

  net::IOBuffer buffer;
  net::HttpProtoService::ResponseToBuffer(response.get(), &buffer);
  LOG(ERROR) << "response:\n"  << buffer.AsString();

  return 0;
}
