#include "bitmap_merger.h"
#include "posting_field/posting_list_manager.h"

namespace component {

BitMapMerger::BitMapMerger(PostingListManager* m) : bitmap_manager_(m) {}

void BitMapMerger::StartMerger() {
  result_pl_ = std::move(bitmap_manager_->CreateMergePostingList());
}

void BitMapMerger::AddMergeBitMap(const BitMapPostingList* pl) {
  intersect_set_.insert(pl);
}

void BitMapMerger::AddMergeBitMaps(const std::vector<BitMapPostingList*>& pls) {
  intersect_set_.insert(pls.begin(), pls.end());
}

void BitMapMerger::AddSubstractBitmap(const BitMapPostingList* pl) {
  substract_set_.insert(pl);
}
void BitMapMerger::AddSubstractBitMaps(
    const std::vector<BitMapPostingList*>& pls) {
  substract_set_.insert(pls.begin(), pls.end());
}

void BitMapMerger::Calculate() {
  for (const auto& pl : intersect_set_) {
    result_pl_->Intersect(pl);
  }
  intersect_set_.clear();

  for (const auto& pl : substract_set_) {
    result_pl_->Substract(pl);
  }
  substract_set_.clear();
}

std::set<int64_t> BitMapMerger::EndMerger() {
  return bitmap_manager_->GetIdsFromPostingList(result_pl_;);
}

}  // namespace component
