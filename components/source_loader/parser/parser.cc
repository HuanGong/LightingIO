
#include "glog/logging.h"
#include "components/source_loader/parser/parser.h"

namespace component {
namespace sl{

const std::string& ValueTypeToString(SourceValueType t) {
  static std::vector<std::string> mapping_vector = {
      "null", "int", "double", "string", "bool", "json", "invalid"
  };
  if (t > SourceValueType::ValueTypeInvalid || t < 0) {
    return mapping_vector[SourceValueType::ValueTypeInvalid];
  }
  return mapping_vector[t];
}
SourceValueType ToSourceValueType(const std::string& str) {
  SourceValueType r = ValueTypeInvalid;
  if (str == "string") {
    r = ValueTypeString;
  } else if (str == "int") {
    r = ValueTypeInt;
  } else if (str == "bool") {
    r = ValueTypeBoolen;
  } else if (str == "json") {
    return ValueTypeJsonObj;
  } else if (str == "double") {
    return ValueTypeDouble;
  } else if (str == "json") {
    return ValueTypeJsonObj;
  }
  return r;
}

Parser::~Parser() {};

bool Parser::Initialize(const Json& config) {
  const static Json nil_value;
  if (!config.is_object()) {
    LOG(WARNING) << __FUNCTION__ << " bad parser config:" << config;
    return false;
  }
  type_ = config.value("type", "");
  primary_ = config.value("primary", "");
  content_mode_ = config.value("content_mode", "line");

  return (!type_.empty()) &&  (!primary_.empty());
};

void Parser::SetDelegate(ParserDelegate* d) {
  delegate_ = d;
};

bool Parser::CheckDefault(SourceValueType t, Json& default_value) {
  if (default_value.is_null()) {
    return true;
  }
  switch (t) {
    case ValueTypeInt:
    case ValueTypeDouble:
      return default_value.is_number();
    case ValueTypeBoolen:
      return default_value.is_boolean();
    case ValueTypeString:
      return default_value.is_string();
    case ValueTypeJsonObj:
      return true; 
    default:
      break;
  }
  return false;
}

}} //end namespace
