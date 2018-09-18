//
// Created by gh on 18-9-17.
//

#include "loader_factory.h"
#include "loader/file_loader.h"
#include "glog/logging.h"

namespace component {
namespace sl {


static base::LazyInstance<LoaderFactory> loader_instance = LAZY_INSTANCE_INIT;
//static
LoaderFactory& LoaderFactory::Instance() {
  return loader_instance.get();
}

Loader* LoaderFactory::CreateLoader(ReaderDelegate* d, Json& conf) {
  const std::string& type = conf.at("type").get<std::string>();
  if (type == "file") {
    return new FileLoader(d, conf);
  } else {
    LOG(ERROR) << "loader type [" << type << "] not supported, conf:" << conf;
  }
  return nullptr;
}

}}


