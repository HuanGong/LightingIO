#ifndef _LT_COMPONENT_INVERTED_INDEX_FIELD_H_
#define _LT_COMPONENT_INVERTED_INDEX_FIELD_H_

#include <set>
#include <memory>
#include <unordered_map>
#include "../expression.h"
#include "bitmap_posting_list.h"

namespace component {

typedef struct PostingList {
  IdList wildcards;
  std::unordered_map<std::string, IdList> include_pl;
  std::unordered_map<std::string, IdList> exclude_pl;
} PostingList;
void to_json(Json& j, const PostingList& pl);

class Field {
public:
  Field(std::string name);
  virtual ~Field();
	const std::string& FieldName() const {return field_name_;}
	const PostingList& GetPostingList() const {return posting_list_;}
  const BitMapPostingList* WildcardsBitMap() const {return wildcards_pl_;}

	void AddWildcards(int64_t wc_doc);
	bool ResolveExpression(const Expression& expr, const Document* doc);
  void AddPostingListValues(const std::string& value, bool is_exclude, int64_t doc);

  //pl relative
  void SetWildcardsPostingList(BitMapPostingList* pl);
	virtual void DumpTo(std::ostringstream& oss) = 0;
  virtual void ResolveValueWithPostingList(const std::string&, bool in, BitMapPostingList*) = 0;
  virtual std::vector<BitMapPostingList*> GetIncludeBitmap(const std::string& value) = 0;
  virtual std::vector<BitMapPostingList*> GetExcludeBitmap(const std::string& value) = 0;
protected:
	std::string field_name_;
	PostingList posting_list_;

  BitMapPostingList* wildcards_pl_ = NULL;
};
typedef std::unique_ptr<Field> FieldPtr;

}
#endif
