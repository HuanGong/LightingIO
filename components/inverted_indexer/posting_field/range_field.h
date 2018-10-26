//
// Created by gh on 18-10-14.
//

#ifndef LIGHTINGIO_RANGE_FIELD_H_H
#define LIGHTINGIO_RANGE_FIELD_H_H

#include "field.h"

namespace component {

class RangeField : public Field {
public:
	RangeField(const std::string&, const std::string& delim);

	void DumpTo(std::ostringstream& oss) override;
  void ResolveValueWithPostingList(const std::string&, bool in, BitMapPostingList*) override;

  std::vector<BitMapPostingList*> GetIncludeBitmap(const std::string& value);
  std::vector<BitMapPostingList*> GetExcludeBitmap(const std::string& value);
private:
  const std::string delim_;
  //return start,end pair
  bool ParseRange(const std::string& assign, int32_t& start, int32_t& end);

  std::unordered_map<int32_t, BitMapPostingList*> include_pl_;
  std::unordered_map<int32_t, BitMapPostingList*> exclude_pl_;
};

} //end component
#endif //LIGHTINGIO_GENERAL_STR_FIELD_H
