//
// Created by gh on 18-9-16.
//

#include "source.h"
#include "glog/logging.h"
#include "loader_factory.h"

namespace component {
namespace sl {

Source::Source(Json& source_config)
	: source_config_(source_config) {
}

Source::~Source() {
}

bool Source::Initialize() {
	Json& loader_config = source_config_.at("loader");
	if (!loader_config.is_object()) {
		LOG(ERROR) << " Init Source Failed with config:" << source_config_;
		return false;
	}
	loader_.reset(LoaderFactory::Instance().CreateLoader(nullptr, loader_config));
	//CHECK(loader_);
	if (!loader_) {
		LOG(ERROR) << " CreateLoader Failed with config:" << loader_config;
		return false;
	}

	return true;
}


}} //component::sl
