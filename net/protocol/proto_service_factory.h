#ifndef NET_PROTOCOL_SERVICE_FACTORY_H
#define NET_PROTOCOL_SERVICE_FACTORY_H

#include <map>
#include <memory>
#include <functional>

#include "../net_callback.h"

namespace net {

typedef std::function<RefProtoService(void)> ProtoserviceCreator;

class ProtoServiceFactory {
public:
  static ProtoServiceFactory& Instance();

  ProtoServiceFactory();
  RefProtoService Create(const std::string& proto);
  // not thread safe,
  // this can cover the default protoservice or add new protocol support
  void RegisterCreator(const std::string, ProtoserviceCreator);
  bool HasProtoServiceCreator();
private:
  void InitInnerDefault();
  std::map<std::string, ProtoserviceCreator> creators_;
};

}
#endif
