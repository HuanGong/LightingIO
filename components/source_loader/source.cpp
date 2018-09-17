//
// Created by gh on 18-9-16.
//

#include "source.h"

namespace component {
namespace sl {

Source::Source(Json& source_config)
	: source_config_(config) {
}

Source::~Source() {
}

bool Source::Initialize() {
	return true;
}


}} //component::sl
