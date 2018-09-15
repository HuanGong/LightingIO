#include <math.h>
#include <inttypes.h>
#include <iostream>
#include "bloom_filter.h"
#include "thirdparty/murmurhash/MurmurHash3.h"

namespace component {

namespace __detail {

BFArgs Calculate(uint64_t n, double p) {
  BFArgs args;
  args.n = n;
  args.p = p;

  uint64_t m = std::ceil(n * std::log(p)) / std::log(1 / std::pow(2, std::log(2)));
  uint32_t k = std::round(double(m / double(n)) * std::log(2));
  args.m = m;
  args.k = k;
  return args;
}

}; //end __detail


BloomFilter::BloomFilter(uint64_t n, double p) {
  args = __detail::Calculate(n, p);
  table_size_ = args.m >> 3;
  table_ = (uint8_t*)malloc(table_size_);
}

BloomFilter::~BloomFilter() {
  if (table_) {
    free(table_);
  }
  table_ = nullptr;
}

void BloomFilter::Set(void* data, size_t len) {
  uint64_t result[2];
  for (uint64_t i = 0; i < args.k;) {
    MurmurHash3_x64_128(data, len, i << 0x1F | i * 0x55, result);
    SetBit(result[0]);
    SetBit(result[1]);
    i+=2;
  }
}

bool BloomFilter::IsSet(void* data, size_t len) {
  uint64_t result[2];
  for (uint64_t i = 0; i < args.k;) {
    MurmurHash3_x64_128(data, len, i << 0x1F | i * 0x55, result);
    if (!TestBit(result[0])) return false;
    if (!TestBit(result[1])) return false;
    i+=2;
  }
  return true;
}

bool BloomFilter::TestAndSet(void* data, size_t len) {
  bool hit = true;
  uint64_t result[2];
  for (uint64_t i = 0; i < args.k;) {
    MurmurHash3_x64_128(data, len, i << 0x1F | i * 0x55, result);
    hit &= TestAndSetBit(result[0]);
    hit &= TestAndSetBit(result[1]);
    i+=2;
  }
  return hit;
}

void BloomFilter::SetBit(uint64_t t) {
  uint64_t index = (t >> 3) % table_size_;
  uint64_t n_bit = t & 0x7;
  __sync_fetch_and_or(table_ + index, 1 << (n_bit));
}

bool BloomFilter::TestBit(uint64_t t) {
  uint64_t index = (t >> 3) % table_size_;
  uint64_t n_bit = t & 0x7;
  return table_[index] & (1 << n_bit);
}

bool BloomFilter::TestAndSetBit(uint64_t t) {
  uint64_t index = (t >> 3) % table_size_;
  uint64_t n_bit = t & 0x7;
  bool hit = table_[index] & (1 << n_bit);
  __sync_fetch_and_or(&table_[index], 1 << (n_bit));
  return hit;
}

};
