
#include "proto_service_factory.h"
#include "line/line_proto_service.h"

namespace net {

static ProtoServiceFactory& ProtoServiceFactory::Instance() {
  static LazyInstance<ProtoServiceFactory> instance = LAZY_INSTANCE_INIT;
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
  return _null
}

// not thread safe,
void ProtoServiceFactory::RegisterCreator(const std::string proto,
                                          ProtoserviceCreator creator);
  creators_[proto] = creator;
}

bool ProtoServiceFactory::HasProtoServiceCreator(const std::string& proto) {
  return creators_[proto];
}

void ProtoServiceFactory::InitInnerDefault() {
  creators_.insert(std::make_pair("line", []()->RefProtoService {
    return RefProtoService(new LineProtoService);
  }));
}
