#ifndef _LT_COMPONENT_INVERTED_INDEX_INDEXER_H_
#define _LT_COMPONENT_INVERTED_INDEX_INDEXER_H_

#include "expression.h"
#include "posting_field/general_str_field.h"
#include "posting_field/posting_list_manager.h"

namespace component {

typedef std::set<std::string> FieldsSet;
typedef std::function<FieldPtr()> FieldCreator;

typedef std::map<std::string, std::vector<std::string>> IndexerQuerys;

class Indexer {
public:
  Indexer();

  std::set<int64_t> UnifyQuery(const IndexerQuerys& query);

  void AddDocument(const RefDocument&& document);
  void RegisterGeneralStrField(const std::string field);
  void RegisterFieldWithCreator(const std::string& f, FieldCreator creator);

  void BuildIndexer();
  void DebugDump(std::ostringstream& oss);
  void DebugDumpField(const std::string& field, std::ostringstream& oss);

private:
  Field* GetPostingListField(const std::string& field);

  bool keep_docments_;
  std::vector<RefDocument> all_documents_;

  FieldsSet indexer_fields_;

  std::set<int64_t> all_documens_ids_;

  std::unordered_map<std::string, FieldPtr> indexer_table_;
  std::unordered_map<std::string, FieldCreator> creators_;
  std::unique_ptr<PostingListManager> posting_list_manager_;
};

}  // namespace component
#endif
