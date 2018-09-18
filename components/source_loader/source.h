//
// Created by gh on 18-9-16.
//

#ifndef LIGHTINGIO_COMPONENT_SOURCE_H
#define LIGHTINGIO_COMPONENT_SOURCE_H

#include "loader.h"
#include <memory>

namespace component {
namespace sl {

class Source {
public:
	Source(Json& source_config);
	virtual ~Source();

	bool Initialize();

	const std::string& Name() const {
		return source_name_;
	}
protected:
	Json source_config_;
	std::string source_name_;

	std::unique_ptr<Loader> loader_;
};

}} //end component::sl
#endif //LIGHTINGIO_SOURCE_H
