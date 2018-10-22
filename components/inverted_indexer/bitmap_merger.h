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
  struct MergerGroup {
    std::set<const BitMapPostingList*> includes_;  //并
    std::set<const BitMapPostingList*> excludes_;  //交
    std::set<const BitMapPostingList*> wildcards_; //差
  };

  BitMapMerger(PostingListManager* m);
  // operator new() = delete;
  void StartMerger();
  std::set<int64_t> EndMerger();

  bool CalculateMergerGroup(MergerGroup& group);

  bool IsFinished() const {return is_done_;}
private:
  void Calculate();

  bool is_done_ = false;
  BitMapPlPtr result_pl_;
  BitMapPlPtr group_result_pl_;

  std::set<const BitMapPostingList*> union_set_;     //并
  std::set<const BitMapPostingList*> intersect_set_; //交
  std::set<const BitMapPostingList*> substract_set_; //差

  PostingListManager* bitmap_manager_;
};

}  // namespace component
#endif
