//
// Created by gh on 18-9-22.
//

#ifndef LIGHTINGIO_ID_STRUCT_SOURCE_H
#define LIGHTINGIO_ID_STRUCT_SOURCE_H

#include "../source.h"
#include <inttypes.h>
#include <unordered_map>
#include <glog/logging.h>
#include "base/base_micro.h"

namespace component {
namespace sl {

template <typename T>
class NameStructSource : public Source {
public:
	explicit NameStructSource(Json& config) : Source(config) {
	};

	const T& GetByName(const char* s) const {
		std::string name(s);
		return GetByName(name);
	}
	const T& GetByName(const std::string& name) const {
		static T kNull;
		if (const auto& it = source_.find(name); it != source_.end()) {
			return it->second;
		}
		return kNull;
	}

	bool Initialize() override {
		if (!Source::Initialize()) return false;
		name_ = source_config_.value("/parser/primary"_json_pointer, "");
		return !name_.empty();
	}

	void OnContentParsed(const Json& data) override {

		Json::const_iterator iter = data.find(name_);
		if (iter == data.end() || !iter->is_string()) {
			LOG(ERROR) << __FUNCTION__ << " Source Can't Find Primary Key for store";
			return;
		}
		std::string name = (*iter);
		source_[name] = data;
	};
private:
	std::string name_;
	std::unordered_map<std::string, T> source_;
	DISALLOW_COPY_AND_ASSIGN(NameStructSource<T>);
};

}}

#endif //LIGHTINGIO_ID_STRUCT_SOURCE_H
