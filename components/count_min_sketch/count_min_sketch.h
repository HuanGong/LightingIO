#ifndef COMPONENT_COUNT_MIN_SKETCH_H_
#define COMPONENT_COUNT_MIN_SKETCH_H_

#include <vector>
#include <string>
#include <inttypes.h>
#include <memory>

namespace component {

//template<class CountType>
#define CountType uint8_t

class CountMinSketch {
public:
  //static std::shared_ptr<CountMinSketch> CreateWithWidthAndDepth(uint64_t w, uint32_t d);
  //static std::shared_ptr<CountMinSketch> CreateWithErrorRateAndCertainty(uint64_t w, uint32_t d);

  CountMinSketch(double error, double certainty);
  ~CountMinSketch();

  void Increase(const std::string& key, int32_t count);
  void Increase(const void* data, size_t len, int32_t count);
  uint64_t Estimate(const std::string& key);
  uint64_t Estimate(const void* data, size_t len);

  const uint64_t Width() const {return width_;};
  const uint64_t Depth() const {return depth_;};
  const uint64_t Certainty() const {return certainty_;};
  const uint64_t ErrorRate() const {return error_rate_;};
  const uint64_t DistinctCount() const {return distinct_count_;};
private:
  double certainty_;
  double error_rate_;

  uint64_t width_;
  uint64_t depth_;

  uint64_t distinct_count_;
  std::vector<CountType*> matrix_;
};

} //end namespace component
#endif
