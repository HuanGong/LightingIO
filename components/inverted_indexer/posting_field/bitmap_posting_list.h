#ifndef LIGHTINGIO_BITMAP_POSTING_LIST_H
#define LIGHTINGIO_BITMAP_POSTING_LIST_H

#include <set>
#include <string>
#include <vector>
#include <memory>
#include <bitset>
#include <sstream>
#include <unordered_map>
#include "glog/logging.h"

namespace component {

class BitMapPostingList {
public:
	BitMapPostingList(int64_t bit_count, int64_t id_count, uint64_t value = 0)
		: id_count_(id_count),
      bit_count_(bit_count) {
		ids_bitmap_.resize((bit_count + 63) / 64, value);
	}
  void ResetBits();
	void SetBit(uint64_t idx);
	void ClearBit(uint64_t idx);
  bool IsBitSet(uint64_t idx) const;
	uint64_t IdCount() const {return id_count_;};
  uint64_t BitCount() const {return bit_count_;};

  void Union(const BitMapPostingList* other);
  void Intersect(const BitMapPostingList* other);
  void Substract(const BitMapPostingList* other);

  std::string DumpBits();
private:
	uint64_t id_count_ = 0;
  uint64_t bit_count_ = 0;
	std::vector<uint64_t> ids_bitmap_;
};
typedef std::unique_ptr<BitMapPostingList> BitMapPlPtr;

} //end namespace

#endif
