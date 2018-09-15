#ifndef _COMPONENT_BLOOMFILTER_H_H_
#define _COMPONENT_BLOOMFILTER_H_H_

#include <vector>
#include <math.h>
#include <inttypes.h>
#include <iostream>

namespace component {

namespace __detail {

typedef struct {
  double  p;
  uint64_t n;
  uint64_t m;
  uint32_t k;
} BFArgs;

/* calculator
   n = ceil(m / (-k / log(1 - exp(log(p) / k))))
   p = pow(1 - exp(-k / (m / n)), k)
   m = ceil((n * log(p)) / log(1 / pow(2, log(2))));
   k = round((m / n) * log(2));
*/
BFArgs Calculate(uint64_t n, double p);
};

class BloomFilter {
public:
  BloomFilter(uint64_t n, double p);
  ~BloomFilter();

  void Set(void* data, size_t len);
  bool IsSet(void* data, size_t len);
  bool TestAndSet(void* data, size_t len);

private:
  // give a number, set a bit according to table_size_;
  void SetBit(uint64_t);
  bool TestBit(uint64_t);
  bool TestAndSetBit(uint64_t);

  uint8_t* table_ = NULL;
  uint64_t table_size_ = 0;
  __detail::BFArgs args;
};

};
#endif
