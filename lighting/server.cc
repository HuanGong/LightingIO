
#include "server.h"

namespace net {

static std::string wokername_prefix("worker_");

uint32_t SrvDelegate::CreateWorkersForServer(WorkerContainer& container) {
  //int hardware_concurrency = std::thread::hardware_concurrency();
  int hardware_concurrency = 24;//std::thread::hardware_concurrency();

  for (int idx = 0; idx < hardware_concurrency; idx++) {
    std::string worker_name = wokername_prefix + std::to_string(idx);
    std::shared_ptr<base::MessageLoop> worker(new base::MessageLoop(worker_name));

    worker->Start();
    container.push_back(std::move(worker));
  }
  return hardware_concurrency;
}

std::vector<std::string> SrvDelegate::HttpListenAddress() {
  std::vector<std::string> http_v;
  http_v.push_back(std::string("0.0.0.0:6666"));
  http_v.push_back(std::string("0.0.0.0:6665"));
  return http_v;
}

std::vector<std::string> SrvDelegate::TcpListenAddress() {
  std::vector<std::string> tcp_v;
  tcp_v.push_back(std::string("0.0.0.0:6667"));
  return tcp_v;
}

}//end namespace
