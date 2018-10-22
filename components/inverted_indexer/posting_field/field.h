#ifndef _LT_COMPONENT_INVERTED_INDEX_FIELD_H_
#define _LT_COMPONENT_INVERTED_INDEX_FIELD_H_

#include <set>
#include <memory>
#include <unordered_map>
#include "../expression.h"
#include "bitmap_posting_list.h"

namespace component {

class Field {
public:
  Field(std::string name);
  virtual ~Field();
	const std::string& FieldName() const {return field_name_;}

  //pl relative
  void SetWildcardsPostingList(BitMapPostingList* pl);
  const BitMapPostingList* WildcardsBitMap() const {return wildcards_pl_;}

  virtual std::vector<BitMapPostingList*> GetIncludeBitmap(const std::string& value) = 0;
  virtual std::vector<BitMapPostingList*> GetExcludeBitmap(const std::string& value) = 0;
  virtual void ResolveValueWithPostingList(const std::string&, bool in, BitMapPostingList*) = 0;

	virtual void DumpTo(std::ostringstream& oss) = 0;
protected:
	std::string field_name_;
  BitMapPostingList* wildcards_pl_ = NULL;
};
typedef std::unique_ptr<Field> FieldPtr;

/* A Holder for every field, save the meta of a AssignValue to DocumentIdList
 *
 * FieldEntity be used for building a FieldPostingField
 * every assianvalue will be parsed and build as PostingList
 *  
 *  Field -> AssignValue: DocumentIdList
 *  eg
 *  Field: ip
 *               "127.0.0.1"    -> [1, 4]
 *               "12.2.11.1"    -> [1, 2, 3]
 *               "192.168.1.*"  -> [1, 2, 3, 4]   // this meta will parse to 192.168.1.1-192.168.1.255 map to [1, 2, 3, 4]
 *
 * It's will managered by indexer, and may be destroied for memory save reason
 * */
typedef struct FieldEntity {
  void AddWildcardsId(DocIdType id);
  void AddIdForAssign(const std::string assign, bool exclude, DocIdType id);

  IdList wildcards;
  std::unordered_map<std::string, IdList> include_ids;
  std::unordered_map<std::string, IdList> exclude_ids;
} FieldEntity;
void to_json(Json& json, const FieldEntity& entity);

} //end namespace
#endif
