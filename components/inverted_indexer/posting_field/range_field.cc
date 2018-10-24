//
// Created by gh on 18-10-14.
//
#include <iostream>
#include <sstream>
#include "general_str_field.h"

namespace component {

RangeField::RangeField(const std::string& field)
  : Field(field) {
}

void RangeField::DumpTo(std::ostringstream& oss) {
	Json out;
	out["field"] = FieldName();
	oss << out;
}

void RangeField::ResolveValueWithPostingList(const std::string& v, bool in, BitMapPostingList* pl) {
  in ? include_pl_[v] = pl : exclude_pl_[v] = pl;
}

std::vector<BitMapPostingList*> RangeField::GetIncludeBitmap(const std::string& value) {
  std::vector<BitMapPostingList*> result;
  auto iter = include_pl_.find(value);
  if (iter != include_pl_.end()) {
    result.push_back(iter->second);
  }
  return std::move(result);
}

std::vector<BitMapPostingList*> RangeField::GetExcludeBitmap(const std::string& value) {
  std::vector<BitMapPostingList*> result;
  auto iter = exclude_pl_.find(value);
  if (iter != exclude_pl_.end()) {
    result.push_back(iter->second);
  }
  return std::move(result);
}

std::pair<int32_t, int32_t> ParseRange(const std::string& exp) {

}

} //end namespace component

