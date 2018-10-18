//
// Created by gh on 18-10-14.
//

#ifndef LIGHTINGIO_GENERAL_STR_FIELD_H
#define LIGHTINGIO_GENERAL_STR_FIELD_H

#include "field.h"

namespace component {

class GeneralStrField : public Field {
public:
	GeneralStrField(const std::string&);

	void DumpTo(std::ostringstream& oss) override;
  void ResolveValueWithPostingList(const std::string&, bool in, BitMapPostingList*) override;

  std::vector<BitMapPostingList*> GetIncludeBitmap(const std::string& value);
  std::vector<BitMapPostingList*> GetExcludeBitmap(const std::string& value);
private:
  std::unordered_map<std::string, BitMapPostingList*> include_pl_;
  std::unordered_map<std::string, BitMapPostingList*> exclude_pl_;
};

} //end component
#endif //LIGHTINGIO_GENERAL_STR_FIELD_H
