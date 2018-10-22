#include <iostream>
#include "bitmap_merger.h"
#include "posting_field/posting_list_manager.h"

namespace component {

BitMapMerger::BitMapMerger(PostingListManager* m) : bitmap_manager_(m) {}

void BitMapMerger::StartMerger() {
  result_pl_ = std::move(bitmap_manager_->CreateMergePostingList());
  group_result_pl_ = std::move(bitmap_manager_->CreateMergePostingList());
}

std::set<int64_t> BitMapMerger::EndMerger() {
  return bitmap_manager_->GetIdsFromPostingList(result_pl_.get());
}

bool BitMapMerger::CalculateMergerGroup(MergerGroup& group) {
  group_result_pl_->ResetBits();

  for (const auto& include_pl : group.includes_) {
    group_result_pl_->Union(include_pl);
  }
  Json o = bitmap_manager_->GetIdsFromPostingList(group_result_pl_.get());
  std::cout << "\t >>> grp after union includes:\t" << o << std::endl;

  for (const auto& wildcard : group.wildcards_) {
    group_result_pl_->Union(wildcard);
  }
  o = bitmap_manager_->GetIdsFromPostingList(group_result_pl_.get());
  std::cout << "\t >>> grp after union wildcard:\t" << o << std::endl;

  for (const auto& exclude_pl : group.excludes_) {
    group_result_pl_->Substract(exclude_pl);
  }
  o = bitmap_manager_->GetIdsFromPostingList(group_result_pl_.get());
  std::cout << "\t >>> grp after substract excludes:\t" << o << std::endl;

  result_pl_->Intersect(group_result_pl_.get());
  return true;
}

}  // namespace component
