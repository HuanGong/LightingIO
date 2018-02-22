
#include "memory/lazy_instance.h"
#include "proto_service_factory.h"
#include "line/line_proto_service.h"
#include "http/http_proto_service.h"
#include "raw/raw_proto_service.h"

namespace net {

//static
ProtoServiceFactory& ProtoServiceFactory::Instance() {
  static base::LazyInstance<ProtoServiceFactory> instance = LAZY_INSTANCE_INIT;
  return instance.get();
}

ProtoServiceFactory::ProtoServiceFactory() {
  InitInnerDefault();
}

RefProtoService ProtoServiceFactory::Create(const std::string& proto) {
  static RefProtoService _null;
  if (creators_[proto]) {
    return creators_[proto]();
  }
  LOG(ERROR) << __FUNCTION__ << " Protocol:" << proto << " Not Supported";
  return _null;
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
  creators_.insert(std::make_pair("line", []()->RefProtoService {
    return RefProtoService(new LineProtoService);
  }));
  creators_.insert(std::make_pair("http", []()->RefProtoService {
    return RefProtoService(new HttpProtoService);
  }));
  creators_.insert(std::make_pair("raw", []()->RefProtoService {
    return RefProtoService(new RawProtoService);
  }));
}

}//end namespace net
