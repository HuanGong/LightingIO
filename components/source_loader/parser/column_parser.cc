//
// Created by gh on 18-9-18.
//

#include "column_parser.h"
#include <glog/logging.h>
#include "base/base_constants.h"
#include "base/string_utils/string_utils.hpp"

/*
 *   eg: AngleZhou#21#{"email":"gopher@gmail.com"}#extra_what_ever
 *
  "parser": {
      "type": "column",
      "delimiter":"#",
      "primary": "name",
      "content_mode": "line",
      "ignore_header_line":false,
      "header": [
        "name",
        "age",
        "info",
        "ext"
      ],
      "schemes": [
        {
          "name": "name",
          "type": "string",
          "default": "",
          "allow_null": false
        },
        {
          "name": "age",
          "type": "int",
          "default": 5,
          "allow_null": true
        },
        {
          "name": "info",
          "type": "json",
          "default": {},
          "allow_null": true
        }
      ]
    },
  }*/

namespace component {
namespace sl{

void to_json(Json& j, const ColumnInfo& info) {
  if (info.IsNull()) {
    j = Json(Json::value_t::object);
  }
  j["name"] = info.name;
  j["type"] = ValueTypeToString(info.type);
  j["default"] = info.default_value;
  j["allow_null"] = info.allow_null;
}
void from_json(const Json& j, ColumnInfo& info) {
  if (!j.is_object()) {
    info.SetNull(true);
    return;
  }
  try {
    info.name = j["name"];
    info.allow_null = j["allow_null"];
    info.default_value = j["default"];
    info.type = ToSourceValueType(j["type"]);
    info.SetNull(false);
  } catch (...) {
    info.SetNull(true);
  }
}

ColumnParser::~ColumnParser() {
}

bool ColumnParser::ParseContent(const std::string& content) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";

  std::vector<std::string> results = str::split(content, delimiter_);
  if (results.size() != header_.size()) {
    LOG(INFO) << __FUNCTION__ << " bad content:" << content << " delemiter:" << delimiter_;
    return false;
  }

  Json column_out;
  for (size_t i = 0; i < results.size(); i++) {

    auto iter = columns_.find(header_[i]);
    if (iter == columns_.end()) {
      continue;
    }

    if (!HanleColumn(iter->second, results[i], column_out)) {
      LOG(INFO) << __FUNCTION__ << " parse field [" << header_[i] << "] Failed, content:" << results[i];
      return false;
    }
  }
  if (delegate_) {
    delegate_->OnContentParsed(column_out);
  }

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " leave";
  return true;
}

bool ColumnParser::HanleColumn(const ColumnInfo& info, const std::string& content, Json& out) {
  std::string field = info.name;
  switch (info.type) {
    case ValueTypeBoolen:
      out[field] = (content == "true");
      break;
    case ValueTypeDouble:
      out[field] = std::atof(content.data());
      break;
    case ValueTypeInt:
      out[field] = std::atoi(content.data());
      break;
    case ValueTypeJsonObj: {
      Json j = Json::parse(content, nullptr, false);
      if (j.is_discarded()) {
        out[field] = info.default_value;
      } else {
        out[field] = std::move(j);
      }
    } break;
    case ValueTypeString:
      out[field] = content;
      break;
    default:
      return false;
      break;
  }
  return true;
}

bool ColumnParser::Initialize(const Json& config) {
  const static Json nil_value;
  if (!Parser::Initialize(config)) {
    return false;
  }

  const Json& schemes = config.value("schemes", nil_value);
  if (!schemes.is_array()) {
    LOG(WARNING) << __FUNCTION__ << " bad schemes config:" << config;
    return false;
  }

  for (Json::const_iterator iter = schemes.begin(); iter != schemes.end(); iter++) {
  	if (!iter->is_object()) {
      LOG(WARNING) << __FUNCTION__ << " bad scheme:" << *iter;
      return false;
  	}
    ColumnInfo info = (*iter);
  	if (info.IsNull()) {
      LOG(INFO) << __FUNCTION__ << " bad scheme from_json:" << *iter;
      return false;
  	}
    if (!CheckDefault(info.type, info.default_value)) {
      LOG(INFO) << __FUNCTION__ << " bad scheme default field:" << info.default_value;
      return false;
    }
    LOG(INFO) << __FUNCTION__ << " add column scheme:" << info.name;
    columns_[info.name] = info;
  }

  delimiter_ = config.value("delimiter", "");
  if (delimiter_.empty()) {
    LOG(INFO) << "column parser must need a delimiter:" << config;
    return false;
  }

  const Json& header = config.value("header", nil_value);
  if (!header.is_array()) {
    LOG(INFO) << "column meta header not found:" << config;
    return false;
  }
  header_ = header.get<std::vector<std::string> >();
  if (header_.empty()) {
    LOG(INFO) << "column parser header field not correct:" << header;
    return false;
  }

  if (const auto it = config.find("ignore_header_line"); it == config.end()) {
    ignore_header_line_ = false;
  } else {
    ignore_header_line_ = (*it);
  }

  return true;
}

}}//end  namespace
