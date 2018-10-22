//
// Created by gh on 18-10-15.
//

#include <algorithm>
#include <iostream>
#include "thirdparty/crc/crc.h"
#include "posting_list_manager.h"
#include "bitmap_posting_list.h"

namespace component {
#define FULLUINT64BIT 0xFFFFFFFFFFFFFFFF
BitMapPlPtr PostingListManager::CreateMergePostingList() {
  BitMapPlPtr pl(new BitMapPostingList(sorted_doc_ids_.size(), 0, FULLUINT64BIT));
  return std::move(pl);
}

BitMapPostingList* PostingListManager::BuildBitMapPostingListForIds(const std::set<int64_t>& ids) {
	if (ids.empty()) return NULL;

	std::vector<int64_t> to_be_build_ids(ids.begin(), ids.end());
	std::sort(to_be_build_ids.begin(), to_be_build_ids.end());

	uint64_t crc_code = crc64(0, (uint8_t*)&to_be_build_ids[0], sizeof(int64_t)*to_be_build_ids.size());

	auto pl_iter = ids_table_.find(crc_code);
	if (pl_iter != ids_table_.end()) {
		CHECK(ids_table_[crc_code]->IdCount() == to_be_build_ids.size());
		return pl_iter->second.get();
	}

	//calculate minimal bit_size, save memory
	uint64_t max_bit_idx = doc_id_to_bit_idx_[to_be_build_ids[ids.size() -1]];
	BitMapPlPtr new_ids_bitmap(new BitMapPostingList(max_bit_idx + 1, ids.size()));

	for (auto& doc_id : to_be_build_ids) {
		auto iter = doc_id_to_bit_idx_.find(doc_id);
		CHECK(iter != doc_id_to_bit_idx_.end());
		new_ids_bitmap->SetBit(iter->second);
	}

	BitMapPostingList* bitmap_pl = new_ids_bitmap.get();
	ids_table_[crc_code] = std::move(new_ids_bitmap);

	return bitmap_pl;
}

void PostingListManager::SetSortedIndexedIds(const std::vector<int64_t>&& ids) {
	sorted_doc_ids_ = std::move(ids);
	std::sort(sorted_doc_ids_.begin(), sorted_doc_ids_.end());

  for (uint64_t i = 0; i < sorted_doc_ids_.size(); i++) {
    doc_id_to_bit_idx_[sorted_doc_ids_[i]] = i;
    bit_idx_to_doc_id_[i] = sorted_doc_ids_[i];
  }
}

std::set<int64_t> PostingListManager::GetIdsFromPostingList(const BitMapPostingList* pl) {
  std::set<int64_t> result;
  if (!pl) {return result;}

  for (uint64_t i = 0; i <= pl->BitCount(); i++) {
    if (pl->IsBitSet(i) && i < sorted_doc_ids_.size()) {
      result.insert(sorted_doc_ids_[i]);
    }
  }
  return std::move(result);
}

}
