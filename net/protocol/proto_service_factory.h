#ifndef NET_PROTOCOL_SERVICE_FACTORY_H
#define NET_PROTOCOL_SERVICE_FACTORY_H

#include <map>
#include <memory>
#include <functional>
#include "proto_service.h"
#include "../net_callback.h"

namespace net {

typedef std::function<RefProtoService()> ProtoserviceCreator;

class ProtoServiceFactory {
public:
  static ProtoServiceFactory& Instance();
  static RefProtoService Create(const std::string& proto);

  ProtoServiceFactory();
  // not thread safe,
  // this can cover the default protoservice or add new protocol support
  void RegisterCreator(const std::string, ProtoserviceCreator);
  bool HasProtoServiceCreator(const std::string&);
private:
  void InitInnerDefault();
  std::map<std::string, ProtoserviceCreator> creators_;
};

}
#endif
