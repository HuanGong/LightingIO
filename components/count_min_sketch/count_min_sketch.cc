
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <iostream>
#include "count_min_sketch.h"
#include "thirdparty/murmurhash/MurmurHash3.h"

namespace component {
/*
//static
CountMinSketch::CreateWithWidthAndDepth(uint64_t w, uint32_t d) {
}
static CreateWithErrorRateAndCertainty(uint64_t w, uint32_t d);
*/

/* d=[ln(1−certainty)/ln(1/2)] */
CountMinSketch::CountMinSketch(double err, double certainty) :
  certainty_(certainty),
  error_rate_(err),
  distinct_count_(0) {

  width_ = std::ceil(2.0 / err);
  depth_ = std::ceil(std::log(1.0 - certainty_) / std::log(1.0/2.0));

  for (uint32_t i = 0 ; i < depth_; i++) {
    CountType* d = (CountType*)malloc(width_ * sizeof(CountType));
    memset(d, 0, width_*sizeof(CountType));
    matrix_.push_back(d);
  }
}

CountMinSketch::~CountMinSketch() {
  for (auto* p : matrix_) {
    free(p);
  }
  matrix_.clear();
}

void CountMinSketch::Increase(const std::string& key, int32_t count) {
  Increase(key.data(), key.size(), count);
}

void CountMinSketch::Increase(const void* data, size_t len, int32_t count) {
  uint64_t result[2];
  bool brand_new = true;
  for (uint32_t depth = 0; depth < depth_;) {
    MurmurHash3_x64_128(data, len, depth << 0x1F | depth, result);
    brand_new = (0 == __sync_fetch_and_add(&matrix_[depth++][result[0] % width_], count));
    if (depth < depth_) {
      brand_new = (0 == __sync_fetch_and_add(&matrix_[depth++][result[1] % width_], count));
    }
  }
  if (brand_new) {
    distinct_count_++;
  }
}

uint64_t CountMinSketch::Estimate(const std::string& key) {
  return Estimate(key.data(), key.size());
}

uint64_t CountMinSketch::Estimate(const void* data, size_t len) {
  uint64_t result = 0xFFFFFFFFFFFFFFFF;

  uint64_t hash[2];
  for (uint32_t depth = 0; depth < depth_;) {
    MurmurHash3_x64_128(data, len, depth << 0x1F | depth, hash);
    result = std::min(uint64_t(matrix_[depth++][hash[0] % width_]), result);
    if (depth < depth_) {
      result = std::min(uint64_t(matrix_[depth++][hash[1] % width_]), result);
    }
  }
  return result;
}

} //end component
