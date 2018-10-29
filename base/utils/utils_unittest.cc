#include <catch/catch.hpp>

#include "string/str_utils.hpp"


TESTCASE("string_utils", "[]") {

  std::string str_double("742.16");
  double num_double = base::StrUtils::Parse<double>(str_double);
  CHECK(num_double == 742.16);
}
