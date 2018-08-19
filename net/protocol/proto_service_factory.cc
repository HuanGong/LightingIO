
#include "proto_service_factory.h"

#include "memory/lazy_instance.h"
#include "redis/resp_service.h"
#include "raw/raw_proto_service.h"
#include "line/line_proto_service.h"
#include "http/http_proto_service.h"

namespace net {

//static
ProtoServiceFactory& ProtoServiceFactory::Instance() {
  static base::LazyInstance<ProtoServiceFactory> instance = LAZY_INSTANCE_INIT;
  return instance.get();
}

ProtoServiceFactory::ProtoServiceFactory() {
  InitInnerDefault();
}

//Static
ProtoServicePtr ProtoServiceFactory::Create(const std::string& proto) {
  static ProtoServicePtr _null;

  auto& builder = Instance().creators_[proto];
  if (builder) {
    return builder();
  }
  LOG(ERROR) << __FUNCTION__ << " Protocol:" << proto << " Not Supported";
  return NULL;
}

// not thread safe,
void ProtoServiceFactory::RegisterCreator(const std::string proto,
                                          ProtoserviceCreator creator) {
  creators_[proto] = creator;
}

bool ProtoServiceFactory::HasProtoServiceCreator(const std::string& proto) {
  return creators_[proto] ? true : false;;
}

void ProtoServiceFactory::InitInnerDefault() {
  creators_.insert(std::make_pair("line", []()->ProtoServicePtr {
    return ProtoServicePtr(new LineProtoService);
  }));
  creators_.insert(std::make_pair("http", []()->ProtoServicePtr {
    return ProtoServicePtr(new HttpProtoService);
  }));
  creators_.insert(std::make_pair("raw", []()->ProtoServicePtr {
    return ProtoServicePtr(new RawProtoService);
  }));
  creators_.insert(std::make_pair("redis", []()->ProtoServicePtr {
    return ProtoServicePtr(new RespService);
  }));
}

}//end namespace net
