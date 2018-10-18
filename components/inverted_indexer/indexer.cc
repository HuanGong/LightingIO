#include "indexer.h"
#include <iostream>
#include <sstream>
#include "bitmap_merger.h"

namespace component {

Indexer::Indexer() { posting_list_manager_.reset(new PostingListManager); }

std::set<int64_t> Indexer::UnifyQuery(const IndexerQuerys& query) {
  BitMapMerger merger(posting_list_manager_.get());
  merger.StartMerger();

  for (auto& field_pair : indexer_table_) {
    Field* postinglist_field = field_pair.second.get();

    const auto& query_iter = query.find(field_name);
    if (query_iter == query.end()) {
      auto wildcards_pl = field_pair.second->WildcardsBitMap();
      merger.AddMergeBitMap(wildcards_pl);
      continue;
    }

    const auto& query_values = query_iter->second;

    for (const std::string& v : query_values) {
      merger.AddMergeBitMaps(postinglist_field->GetIncludeBitmap(v));
      merger.AddSubstractBitMaps(postinglist_field->GetExcludeBitmap(v));
    }
  }

  return merger.EndMerger();
}

void Indexer::DebugDumpField(const std::string& field,
                             std::ostringstream& oss) {
  auto pair = indexer_table_.find(field);
  if (pair == indexer_table_.end()) {
    oss << "{ \"error:\" \"" << field << " not found\"}\n";
    return;
  }
  pair->second->DumpTo(oss);
}

void Indexer::DebugDump(std::ostringstream& oss) {
  for (auto& pair : indexer_table_) {
    pair.second->DumpTo(oss);
    oss << "\n";
  }
}

void Indexer::AddDocument(const RefDocument&& document) {
  if (keep_docments_) {
    all_documents_.push_back(document);
  }
  all_documens_ids_.insert(document->DocumentId());

  for (auto field_name : indexer_fields_) {
    auto& doc_conditions = document->Conditions();
    auto expr_iter = doc_conditions.find(field_name);

    Field* pl_field = GetPostingListField(field_name);
    if (!pl_field) {
      std::cout << "field:" << field_name
                << " not supported, register it before use";
      continue;
    }

    if (expr_iter == doc_conditions.end()) {  // not found, set wildcards
      pl_field->AddWildcards(document->DocumentId());
    } else {
      pl_field->ResolveExpression(expr_iter->second, document.get());
    }
  }
}

Field* Indexer::GetPostingListField(const std::string& field) {
  if (indexer_fields_.find(field) == indexer_fields_.end()) {
    return NULL;
  }

  auto iter = indexer_table_.find(field);

  if (indexer_table_.end() == iter) {  // create new one
    const auto& creator = creators_.find(field);
    FieldPtr pl_field;
    if (creator != creators_.end()) {
      pl_field = creator->second();
    } else {  // use GeneralStrField
      pl_field.reset(new GeneralStrField(field));
    }
    auto insert_ret =
        indexer_table_.insert(std::make_pair(field, std::move(pl_field)));
    return insert_ret.first->second.get();
  }
  return iter->second.get();
}

void Indexer::RegisterGeneralStrField(const std::string field) {
  indexer_fields_.insert(field);
}
void Indexer::RegisterFieldWithCreator(const std::string& f,
                                       FieldCreator creator) {
  creators_[f] = creator;
  indexer_fields_.insert(f);
}

void Indexer::BuildIndexer() {
  std::vector<int64_t> ids(all_documens_ids_.begin(), all_documens_ids_.end());
  std::sort(ids.begin(), ids.end());
  posting_list_manager_->SetSortedIndexedIds(std::move(ids));

  for (auto& field_pair : indexer_table_) {
    FieldPtr& field_ptr = field_pair.second;
    const std::string& field_name = field_pair.first;

    const PostingList& pl_entity = field_ptr->GetPostingList();

    BitMapPostingList* bitmap_pl =
        posting_list_manager_->BuildBitMapPostingListForIds(
            pl_entity.wildcards);
    field_ptr->SetWildcardsPostingList(bitmap_pl);

    for (auto& value_ids_pair : pl_entity.include_pl) {
      const std::string& value = value_ids_pair.first;
      BitMapPostingList* in_pl =
          posting_list_manager_->BuildBitMapPostingListForIds(
              value_ids_pair.second);
      field_ptr->ResolveValueWithPostingList(value, true, in_pl);
    }

    for (auto& value_ids_pair : pl_entity.exclude_pl) {
      const std::string& value = value_ids_pair.first;
      BitMapPostingList* ex_pl =
          posting_list_manager_->BuildBitMapPostingListForIds(
              value_ids_pair.second);
      field_ptr->ResolveValueWithPostingList(value, true, ex_pl);
    }
  }  // end foreach register field
}

}  // namespace component
