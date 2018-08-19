///
/// test resp unique_value
///

#define CATCH_CONFIG_NO_CPP11
#include <catch.hpp>

#include <resp/unique_value.hpp>

TEST_CASE("resp unique_value", "[unique_value]")
{
  SECTION("create default")
  {
    resp::unique_value uv;
    REQUIRE(uv.type() == resp::ty_null);
    REQUIRE(!uv);
  }

  SECTION("create from integer")
  {
    resp::unique_value uv(1);
    REQUIRE(uv.type() == resp::ty_integer);
    REQUIRE(uv);
    REQUIRE(uv.integer() == 1);
  }

  SECTION("create from string")
  {
    resp::unique_value uv("test string");
    REQUIRE(uv.type() == resp::ty_string);
    REQUIRE(uv);
    REQUIRE(uv.string() == "test string");
  }

  SECTION("create from user given string")
  {
    std::string str("test string");
    resp::unique_value uv(str.data(), str.size(), resp::ty_string);
    REQUIRE(uv.type() == resp::ty_string);
    REQUIRE(uv);
    REQUIRE(uv.string() == str);
  }

  SECTION("create from user given error")
  {
    std::string str("Error message");
    resp::unique_value uv(str.data(), str.size(), resp::ty_error);
    REQUIRE(uv.type() == resp::ty_error);
    REQUIRE(uv);
    REQUIRE(uv.error() == str);
  }

  SECTION("create from user given bulk string")
  {
    std::string str("bulk string");
    resp::unique_value uv(str.data(), str.size(), resp::ty_bulkstr);
    REQUIRE(uv.type() == resp::ty_bulkstr);
    REQUIRE(uv);
    REQUIRE(uv.bulkstr() == str);
  }

  SECTION("create from user given array")
  {
    resp::unique_array<resp::unique_value> array;
    array.reserve(5);
    array.push_back("simply string");
    array.push_back(resp::unique_value("error", resp::ty_error));
    array.push_back(1);
    array.push_back(resp::unique_value("bulk string", resp::ty_bulkstr));
    resp::unique_array<resp::unique_value> arr(2);
    arr.push_back("simply str");
    arr.push_back(2);
    array.push_back(arr);

    resp::unique_value uv(array);
    REQUIRE(array.size() == 0);
    REQUIRE(uv.type() == resp::ty_array);
    REQUIRE(uv);

    array = uv.array();
    REQUIRE(uv.array().size() == 0);
    REQUIRE(array.size() == 5);
  }

  SECTION("unique_value copy")
  {
    resp::unique_array<resp::unique_value> array;
    array.reserve(5);
    array.push_back("simply string");
    array.push_back(resp::unique_value("error", resp::ty_error));
    array.push_back(1);
    array.push_back(resp::unique_value("bulk string", resp::ty_bulkstr));
    resp::unique_array<resp::unique_value> arr(2);
    arr.push_back("simply str");
    arr.push_back(2);
    array.push_back(arr);

    resp::unique_value src(array);
    REQUIRE(array.size() == 0);
    REQUIRE(src.type() == resp::ty_array);
    REQUIRE(src);

    resp::unique_value des;
    resp::unique_value::copy(des, src);

    REQUIRE(des.type() == resp::ty_array);
    REQUIRE(des);
    REQUIRE(src.type() == resp::ty_array);
    REQUIRE(src);

    REQUIRE(des.array()[0] == src.array()[0]);
    REQUIRE(des.array()[1] == src.array()[1]);
    REQUIRE(des.array()[2] == src.array()[2]);
    REQUIRE(des.array()[3] == src.array()[3]);
    REQUIRE(des.array()[4].array()[0] == src.array()[4].array()[0]);
    REQUIRE(des.array()[4].array()[1] == src.array()[4].array()[1]);
  }
}
