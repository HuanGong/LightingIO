#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include "count_min_sketch.h"


#define CATCH_CONFIG_MAIN //only once
#include <catch/catch.hpp>

TEST_CASE("arg calulate", "[estimate and  calculate args]") {
  component::CountMinSketch sketch(0.0000001, 0.99999);
  std::cout << "w:" << sketch.Width() << " d:" << sketch.Depth() << std::endl; 
  REQUIRE(sketch.Depth() <= 20);
}

TEST_CASE("base", "[test the base function]") {

  component::CountMinSketch sketch(0.0000001, 0.99999);
  sketch.Increase("name", 5);
  sketch.Increase("age", 111);
  REQUIRE(5 == sketch.Estimate("name"));
  REQUIRE(111 == sketch.Estimate("age"));
  REQUIRE(2 == sketch.DistinctCount());

  sketch.Increase("age", 1);
  REQUIRE(112 == sketch.Estimate("age"));
  REQUIRE(2 == sketch.DistinctCount());
}

TEST_CASE("false-rate", "[Add New Item To BloomFilter]") {
  component::CountMinSketch sketch(0.0000001, 0.99999);
  std::cout << "w:" << sketch.Width() << " d:" << sketch.Depth() << std::endl; 

  for (uint32_t i = 0; i < 2000000; i++) {
    sketch.Increase(&i, sizeof(uint32_t), 5);
  }
  std::cout << "distinct count:" << sketch.DistinctCount() << std::endl;
  REQUIRE(sketch.DistinctCount());

  int32_t false_estimate = 0;
  for (uint32_t i = 2000000; i < 4000000; i++) {
    if (0 != sketch.Estimate(&i, sizeof(uint32_t))) {
      false_estimate++;
    }
  }
  std::cout << " false estimate count:" << false_estimate << std::endl;
  REQUIRE(false_estimate < 10);

  uint64_t value_err_count = 0;
  for (uint32_t c = 0; c < 2000000; c++) {
    if (5 != sketch.Estimate(&c, sizeof(uint32_t))) {
      value_err_count++;
    }
  }
  std::cout << " value error estimate count:" << value_err_count << std::endl;
  REQUIRE(value_err_count < 10);
}
