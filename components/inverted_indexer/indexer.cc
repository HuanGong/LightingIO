#include "indexer.h"
#include <iostream>
#include <sstream>
#include "bitmap_merger.h"

namespace component {

Indexer::Indexer(Delegate* d)
    : delegate_(d),
      entitys_meta_(new EntitysMeta),
      pl_manager_(new PostingListManager) {}

std::set<int64_t> Indexer::UnifyQuery(const IndexerQuerys& query) {
  BitMapMerger merger(pl_manager_.get());

  merger.StartMerger();

  Json out = merger.EndMerger();
  std::cout << "after start merge result:" << out << std::endl;

  for (auto& field_pair : indexer_table_) {
    BitMapMerger::MergerGroup grp;

    Field* field_ptr = field_pair.second.get();

    const auto& query_iter = query.find(field_pair.first);
    if (query_iter == query.end()) {
      grp.wildcards_.insert(field_ptr->WildcardsBitMap());
      merger.CalculateMergerGroup(grp);
      Json out = merger.EndMerger();
      std::cout << " field:" << field_pair.first << "\t result:" << out
                << std::endl;
      continue;
    }

    const auto& query_values = query_iter->second;
    for (const std::string& v : query_values) {
      auto includes = field_ptr->GetIncludeBitmap(v);
      for (auto& pl : includes) {
        Json j_include = pl_manager_->GetIdsFromPostingList(pl);
        std::cout << " field:" << field_pair.first << "'s value:" << v
                  << " include ids:" << j_include << std::endl;
      }
      grp.includes_.insert(includes.begin(), includes.end());

      auto excludes = field_ptr->GetExcludeBitmap(v);
      for (auto& pl : excludes) {
        Json j_exclude = pl_manager_->GetIdsFromPostingList(pl);
        std::cout << " field:" << field_pair.first << "'s value:" << v
                  << " exclude ids:" << j_exclude << std::endl;
      }
      grp.excludes_.insert(excludes.begin(), excludes.end());
    }
    grp.wildcards_.insert(field_ptr->WildcardsBitMap());
    out = pl_manager_->GetIdsFromPostingList(field_ptr->WildcardsBitMap());
    std::cout << " field:" << field_pair.first << "'s wildcards ids:" << out
              << std::endl;

    merger.CalculateMergerGroup(grp);

    out = merger.EndMerger();
    std::cout << " field:" << field_pair.first << "\t result:" << out
              << std::endl;
  }

  return merger.EndMerger();
}

void Indexer::DebugDumpField(const std::string& field,
                             std::ostringstream& oss) {
  auto pair = entitys_meta_->field_entitys_.find(field);
  if (pair == entitys_meta_->field_entitys_.end()) {
    oss << "{ \"error:\" \"" << field << " not found\"}\n";
    return;
  }
  Json j_entity = pair->second;
  oss << "field:" << field << "\t" << j_entity << "\n";
}

void Indexer::DebugDump(std::ostringstream& oss) {
  for (auto& pair : entitys_meta_->field_entitys_) {
    Json j_entity = pair.second;
    oss << "field:" << pair.first << "\t" << j_entity << "\n";
  }
}

void Indexer::AddDocument(const RefDocument&& document) {
  if (build_finished_) {
    std::cout << " this indexer is finish build, only work for query"
              << std::endl;
    return;
  }

  entitys_meta_->all_documents_.push_back(document);
  entitys_meta_->all_documens_ids_.insert(document->DocumentId());

  for (auto& field_name : delegate_->AllRegisterFields()) {
    DocIdType doc_id = document->DocumentId();
    auto& doc_conditions = document->Conditions();
    auto expr_iter = doc_conditions.find(field_name);

    FieldEntity& entity = entitys_meta_->field_entitys_[field_name];
    // not found or assigns emtpy, set docid as wildcard for this field
    if (expr_iter == doc_conditions.end() ||
        expr_iter->second.Values().empty()) {
      entity.AddWildcardsId(doc_id);
      continue;
    }

    const Expression& expression = expr_iter->second;
    bool is_exclude_expression = expression.IsExclude();
    for (const auto& assign : expression.Values()) {
      entity.AddIdForAssign(assign, is_exclude_expression, doc_id);
    }
    if (is_exclude_expression) {
      entity.AddWildcardsId(doc_id);
    }
  }
}

Field* Indexer::GetPostingListField(const std::string& field) {
  auto iter = indexer_table_.find(field);
  if (iter != indexer_table_.end()) {
    return iter->second.get();
  }

  FieldPtr field_ptr = delegate_->CreatePostinglistField(field);
  auto ret = indexer_table_.insert(std::make_pair(field, std::move(field_ptr)));
  return ret.first->second.get();
}

void Indexer::BuildIndexer() {
  if (build_finished_) {
    return;
  }

  std::vector<DocIdType> ids(entitys_meta_->all_documens_ids_.begin(),
                             entitys_meta_->all_documens_ids_.end());
  std::sort(ids.begin(), ids.end());

  pl_manager_->SetSortedIndexedIds(std::move(ids));

  std::set<std::string> all_register_field = delegate_->AllRegisterFields();
  for (auto& field_name : all_register_field) {
    FieldEntity& entity = entitys_meta_->field_entitys_[field_name];

    Field* field = GetPostingListField(field_name);
    auto bitmap_pl =
        pl_manager_->BuildBitMapPostingListForIds(entity.wildcards);
    field->SetWildcardsPostingList(bitmap_pl);

    for (auto& pair : entity.include_ids) {
      const std::string& assign = pair.first;
      if (pair.second.empty()) {
        continue;
      }
      auto* in_pl = pl_manager_->BuildBitMapPostingListForIds(pair.second);
      CHECK(in_pl);
      field->ResolveValueWithPostingList(assign, true, in_pl);
    }

    for (auto& pair : entity.exclude_ids) {
      const std::string& assign = pair.first;
      if (pair.second.empty()) {
        continue;
      }
      auto* ex_pl = pl_manager_->BuildBitMapPostingListForIds(pair.second);
      CHECK(ex_pl);
      field->ResolveValueWithPostingList(assign, false, ex_pl);
    }
  }  // end foreach register field

  build_finished_ = true;
}

void DefaultDelegate::RegisterGeneralStrField(const std::string field) {
  indexer_fields_.insert(field);
}

void DefaultDelegate::RegisterFieldWithCreator(const std::string& f,
                                               FieldCreator creator) {
  creators_[f] = creator;
  indexer_fields_.insert(f);
}

FieldPtr DefaultDelegate::CreatePostinglistField(const std::string& name) {
  FieldPtr field_ptr;

  const auto& creator = creators_.find(name);
  if (creator != creators_.end()) {
    field_ptr = creator->second();
  } else {  // use GeneralStrField
    field_ptr.reset(new GeneralStrField(name));
  }
  return std::move(field_ptr);
};

const std::set<std::string> DefaultDelegate::AllRegisterFields() const {
  return indexer_fields_;
}

}  // namespace component
