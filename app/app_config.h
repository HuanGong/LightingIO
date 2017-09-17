#ifdef APP_CONFIG_H_
#define APP_CONFIG_H_
#include <vector>
#include <string>

namespace conf {

struct service_conf {
  int port;
  std::string addr;
  std::string name;
};

class AppConfig {
public:
  AppConfig();
  static AppConfig& Default();
public:
  int worker_numbers;
  std::vector<service_conf> services;
};

}
#endif
