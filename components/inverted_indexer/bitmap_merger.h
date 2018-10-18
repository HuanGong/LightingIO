#ifndef _LT_COMPONENT_BITMAP_MERGER_H_
#define _LT_COMPONENT_BITMAP_MERGER_H_

#include <set>
#include "expression.h"
#include "posting_field/bitmap_posting_list.h"
#include "posting_field/general_str_field.h"

namespace component {

class PostingListManager;

class BitMapMerger {
public:
  BitMapMerger(PostingListManager* m);
  // operator new() = delete;
  void StartMerger();

  void AddMergeBitMap(const BitMapPostingList* pl);
  void AddMergeBitMaps(const std::vector<BitMapPostingList*>& pls);

  void AddSubstractBitmap(const BitMapPostingList* pl);
  void AddSubstractBitMaps(const std::vector<BitMapPostingList*>& pls);
  
  std::set<int64_t> EndMerger();
  bool IsFinished() const {return is_done_;}
private:
  void Calculate();

  bool is_done_ = false;
  BitMapPlPtr result_pl_;
  std::set<const BitMapPostingList*> intersect_set_;
  std::set<const BitMapPostingList*> substract_set_;

  PostingListManager* bitmap_manager_;
};

}  // namespace component
#endif
