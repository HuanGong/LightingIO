///
/// test resp unique_array
///

#define CATCH_CONFIG_NO_CPP11
#include <catch.hpp>

#include <resp/unique_array.hpp>
#include <string>

TEST_CASE("resp unique_array", "[unique_array]")
{
  SECTION("create")
  {
    resp::unique_array<std::string> arr;
    REQUIRE(arr.size() == 0);

    arr.push_back("test string");
    REQUIRE(arr.size() == 1);
    REQUIRE(arr[0] == "test string");

    arr.push_back("test str2");
    REQUIRE(arr.size() == 2);
    REQUIRE(arr[0] == "test string");
    REQUIRE(arr[1] == "test str2");

    arr.clear();
  }

  SECTION("reserve")
  {
    resp::unique_array<std::string> arr;
    arr.reserve(5);
    REQUIRE(arr.size() == 0);

    arr.push_back("test res");
    REQUIRE(arr.size() == 1);

    arr.reserve(5 + RESP_UNIQUE_ARRAY_NODE_CAPACITY);
    REQUIRE(arr.size() == 1);

    arr.push_back("test string 2");
    REQUIRE(arr.size() == 2);
  }

  SECTION("move")
  {
    resp::unique_array<std::string> arr1;
    arr1.push_back("test string");
    arr1.push_back("test str2");
    REQUIRE(arr1.size() == 2);

    resp::unique_array<std::string> arr2(arr1);
    REQUIRE(arr1.size() == 0);

    arr1 = arr2;
    REQUIRE(arr1.size() == 2);
    REQUIRE(arr1[0] == "test string");
    REQUIRE(arr1[1] == "test str2");
    REQUIRE(arr2.size() == 0);

    arr2 = arr1;
    REQUIRE(arr1.size() == 0);
    REQUIRE(arr2.size() == 2);
    REQUIRE(arr2[0] == "test string");
    REQUIRE(arr2[1] == "test str2");
  }
}
