//
// Created by gh on 18-9-21.
//

#ifndef LIGHTINGIO_SAFE_JSON_H
#define LIGHTINGIO_SAFE_JSON_H

#include "json.hpp"

using Json = nlohmann::json;
namespace safejson {

const static Json KDiscardedJson(Json::value_t::discarded);
const static std::string KEmptyString;

const Json& Get(const Json& j, const std::string item) {
	Json::const_iterator it = j.find(item);
	if (it == j.end()) return KDiscardedJson;
	return *it;
}

std::string GetJsonString(const Json& j) {
	if (!j.is_string()) return KEmptyString;
	return j;
}

const std::string& GetJsonRefString(const Json& j) {
	if (j.is_string()) return j.get_ref<const std::string&>();
	return KEmptyString;
}

}

#endif //LIGHTINGIO_SAFE_JSON_H
