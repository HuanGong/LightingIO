#ifndef _LT_COMPONENT_INVERTED_INDEX_INDEXER_H_ 
#define _LT_COMPONENT_INVERTED_INDEX_INDEXER_H_

#include "expression.h"

namespace component {

typedef struct PostingList {
  IdList wildcards;
  std::unordered_map<std::string, IdList> include_pl;
  std::unordered_map<std::string, IdList> exclude_pl;
} PostingList;

class Indexer {
public:
  void Query();
  PostingList& GetPostingListForField(const std::string field) {
    return indexer_table_[field];
  }
  void DebugDump();
private:
  std::unordered_map<std::string, PostingList> indexer_table_;
};

class IndexerBuilder {
public:
  void AddDocument(const RefDocument&& document);
  std::shared_ptr<Indexer> Build();
private:
  std::set<std::string> fields_;
  std::vector<RefDocument> all_documents_;
};


}
#endif
