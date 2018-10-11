#include "indexer.h"
#include <iostream>
#include <sstream>

namespace component {

void Indexer::Query() {

}

void Indexer::DebugDump() {
  //std::unordered_map<std::string, PostingList> indexer_table_;
  std::ostringstream oss;
  for (auto& pair : indexer_table_) {

    oss << "field:" << pair.first << " >>>>>>>>>>>>>>>>>>>>>>>>>>\n{\n";
    Json j_i = pair.second.include_pl;
    oss << "\tinclude:" << j_i << "\n";
    Json j_e = pair.second.exclude_pl;
    oss << "\texclude:" << j_e << "\n";
    Json j_w = pair.second.wildcards;
    oss << "\twildcard:" << j_w << "\n";
    oss << "}\n <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  }
  std::cout << oss.str() << std::endl;
}

void IndexerBuilder::AddDocument(const RefDocument&& document) {
  all_documents_.push_back(document);
}

std::shared_ptr<Indexer> IndexerBuilder::Build() {
  std::shared_ptr<Indexer> indexer(new Indexer);

  for (const RefDocument& doc : all_documents_) {

    int64_t doc_id = doc->DocumentId();

    for (const auto& field_exps_pair : doc->Conditions()) {

      const std::string& field = field_exps_pair.first;
      const Expression& expression = field_exps_pair.second;

      fields_.insert(field);
      PostingList& pl = indexer->GetPostingListForField(field);

      bool is_exclude = expression.IsExclude();

      for (const auto& v : expression.Values()) {
        if (is_exclude) {
          pl.exclude_pl[v].insert(doc_id);
        } else {
          pl.include_pl[v].insert(doc_id);
        }
      }
      if (expression.Values().empty()) {
        pl.wildcards.insert(doc_id);
      }

    } // end foreach Expression in a document

  } //end foreach document

  // build wildcard
  for (const std::string& field : fields_) {

    PostingList& pl = indexer->GetPostingListForField(field);
    for (const RefDocument& doc : all_documents_) {
      if (!doc->HasConditionExpression(field)) {
        pl.wildcards.insert(doc->DocumentId());
      }
    }
  }
  return std::move(indexer);
}



}
