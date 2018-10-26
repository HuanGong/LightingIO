//
// Created by gh on 18-10-14.
//
#include "range_field.h"
#include <iostream>
#include <sstream>
#include "base/string_utils/string_utils.hpp"

namespace component {

RangeField::RangeField(const std::string& field, const std::string& delim)
    : Field(field), delim_(delim) {}

void RangeField::DumpTo(std::ostringstream& oss) {
  Json out;
  out["field"] = FieldName();
  oss << out;
}

void RangeField::ResolveValueWithPostingList(const std::string& v, bool include,
                                             BitMapPostingList* pl) {
  int32_t start = 0;
  int32_t end = 0;

  bool success = ParseRange(v, start, end);
  if (success || start > end) {
    return;
  }

  for (int32_t assign = start; assign <= end; assign++) {
    include ? include_pl_[assign] = pl : exclude_pl_[assign] = pl;
  }
}

std::vector<BitMapPostingList*> RangeField::GetIncludeBitmap(
    const std::string& value) {
  std::vector<BitMapPostingList*> result;

  int32_t assign = 0;
  if (!base::StringUtils::TryParseTo(value, assign)) {
    return result;
  }

  auto iter = include_pl_.find(assign);
  if (iter != include_pl_.end()) {
    result.push_back(iter->second);
  }
  return std::move(result);
}

std::vector<BitMapPostingList*> RangeField::GetExcludeBitmap(
    const std::string& value) {
  std::vector<BitMapPostingList*> result;

  int32_t assign = 0;
  if (!base::StringUtils::TryParseTo(value, assign)) {
    return result;
  }

  auto iter = exclude_pl_.find(assign);
  if (iter != exclude_pl_.end()) {
    result.push_back(iter->second);
  }
  return std::move(result);
}

bool RangeField::ParseRange(const std::string& assign, int32_t& start,
                            int32_t& end) {
  auto range = base::StringUtils::Split(assign, delim_);
  switch (range.size()) {
    case 0:
      return false;
    case 1: {
      bool ok = base::StringUtils::TryParseTo(range[0], start);
      if (ok) {
        end = start;
      }
      return ok;
    } break;
    case 2: {
      return base::StringUtils::TryParseTo(range[1], end) &&
        base::StringUtils::TryParseTo(range[0], start);
    } break;
    default:
      break;
  }
  return false;
}

}  // end namespace component
