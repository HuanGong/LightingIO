//
// Created by gh on 18-10-14.
//
#include <iostream>
#include <sstream>
#include "general_str_field.h"

namespace component {

GeneralStrField::GeneralStrField(const std::string& field)
  : Field(field) {
}

void GeneralStrField::DumpTo(std::ostringstream& oss) {
	Json out;
	out["field"] = FieldName();
	out["posting_list"] = GetPostingList();
	oss << out;
}

void GeneralStrField::ResolveValueWithPostingList(const std::string& v, bool in, BitMapPostingList* pl) {
  in ? include_pl_[v] = pl : exclude_pl_[v] = pl;
}

std::vector<BitMapPostingList*> GeneralStrField::GetIncludeBitmap(const std::string& value) {
  std::vector<BitMapPostingList*> result;
  auto iter = include_pl_.find(value);
  if (iter != include_pl_.end()) {
    result.push_back(iter->second);
  }
  return std::move(result);
}

std::vector<BitMapPostingList*> GeneralStrField::GetExcludeBitmap(const std::string& value) {
  std::vector<BitMapPostingList*> result;
  auto iter = exclude_pl_.find(value);
  if (iter != exclude_pl_.end()) {
    result.push_back(iter->second);
  }
  return std::move(result);
}

} //end namespace component
