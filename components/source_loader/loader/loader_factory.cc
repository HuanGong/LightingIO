//
// Created by gh on 18-9-17.
//

#include "components/source_loader/loader/loader_factory.h"
#include "file_loader.h"
#include "glog/logging.h"

namespace component {
namespace sl {


static base::LazyInstance<LoaderFactory> loader_instance = LAZY_INSTANCE_INIT;
//static
LoaderFactory& LoaderFactory::Instance() {
  return loader_instance.get();
}

Loader* LoaderFactory::CreateLoader(LoaderDelegate* d,const Json& conf) {
  const std::string& type = conf.at("type").get<std::string>();
  if (type == "file") {
    return new FileLoader(d, conf);
  } else {
    LOG(ERROR) << "loader type [" << type << "] not supported, conf:" << conf;
    CHECK(false);
  }
  return nullptr;
}

}} //end namespace


