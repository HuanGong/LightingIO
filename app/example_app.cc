
#include "example_app.h"

#include <fstream>
#include <iostream>
#include <glog/logging.h>
#include <thirdparty/json/json.hpp>
#include <gflags/gflags.h>

#include <net/net_callback.h>
#include <net/url_string_utils.h>
#include <net/protocol/proto_message.h>

DEFINE_string(config, "./manifest.json", "The application Manifest config file");

void HanleMessage(net::RefProtocolMessage message) {
  LOG(INFO) << "Got A INCOMMING Message";
}

namespace content {

GeneralServerApp::GeneralServerApp() {
  LoadConfig();
}

GeneralServerApp::~GeneralServerApp() {

}

void GeneralServerApp::ContentMain() {
  App::ContentMain();

  server_.reset(new net::Server(this));

  json j_service = manifest_["Service"];
  if (j_service.is_array()) {
    for (auto& server : j_service) {
      std::string address = server["address"];
      std::string name = server["name"];
      server_->RegisterService(address, std::bind(HanleMessage, std::placeholders::_1));
    }
  }
  json j_clients = manifest_["clients"];
  if (j_service.is_array()) {
    for (auto& client : j_clients) {
      std::string address = client["address"];

      net::url::SchemeIpPort sch_ip_port;
      if (!net::url::ParseSchemeIpPortString(address, sch_ip_port)) {
        LOG(ERROR) << "argument format error,eg [scheme://xx.xx.xx.xx:port]";
        continue;
      }
      net::RouterConf config;

      config.protocol = sch_ip_port.scheme;

      config.connections = client["connections"];
      config.message_timeout = client["query_timeout"];
      config.recon_interal = client["recon_interval"];

      std::string name = client["name"];

      net::InetAddress server_address(sch_ip_port.ip, sch_ip_port.port);
      std::shared_ptr<net::ClientRouter> router(new net::ClientRouter(MainLoop(), server_address));
      router->SetDelegate(server_.get());
      router->SetupRouter(config);

      router->StartRouter();
      clients_[name] = router;
    }
  }

  //TODO: before application Run

  server_->RunAllService();
}

void GeneralServerApp::LoadConfig() {

  std::ifstream f_manifest(FLAGS_config.c_str());
  CHECK(f_manifest.is_open());

  f_manifest >> manifest_;

  LOG(INFO) << "Application Using Config:\n" <<  manifest_.dump(2);
}

}//end content

