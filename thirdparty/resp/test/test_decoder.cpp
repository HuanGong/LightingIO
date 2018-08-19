///
/// test resp decoder
///

#define CATCH_CONFIG_NO_CPP11
#include <catch.hpp>
#include <resp/decoder.hpp>
#include <iostream>
#include <cstdio>

TEST_CASE("resp decoder", "[decoder]")
{
  resp::decoder dec;

  SECTION("default")
  {
    char buff[32] = "+OK\r\n";
    resp::result res = dec.decode(buff, std::strlen(buff));
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == std::strlen(buff));

    resp::unique_value rep = res.value();
    REQUIRE(rep.type() == resp::ty_string);
    REQUIRE(rep.string() == "OK");
  }

  SECTION("pipeling")
  {
    char buff[32] = "+OK\r\n:1000\r\n-ERR errmsg\r\n";
    resp::unique_value rep;
    resp::result res;

    res = dec.decode(buff, 5);
    rep = res.value();
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == 5);
    REQUIRE(rep.type() == resp::ty_string);
    REQUIRE(rep.string() == "OK");

    res = dec.decode(buff + 5, 7);
    rep = res.value();
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == 7);
    REQUIRE(rep.type() == resp::ty_integer);
    REQUIRE(rep.integer() == 1000);

    res = dec.decode(buff + 5 + 7, 13);
    rep = res.value();
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == 13);
    REQUIRE(rep.type() == resp::ty_error);
    REQUIRE(rep.error() == "ERR errmsg");
  }

  SECTION("pipeling2")
  {
    char buff[32] = "+OK\r\n:1000\r\n-ERR errmsg\r\n";
    size_t len = std::strlen(buff);
    size_t decoded_size = 0;
    resp::unique_value rep;
    resp::result res;

    res = dec.decode(buff + decoded_size, len - decoded_size);
    rep = res.value();
    decoded_size += res.size();
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == 5);
    REQUIRE(rep.type() == resp::ty_string);
    REQUIRE(rep.string() == "OK");

    res = dec.decode(buff + decoded_size, len - decoded_size);
    rep = res.value();
    decoded_size += res.size();
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == 7);
    REQUIRE(rep.type() == resp::ty_integer);
    REQUIRE(rep.integer() == 1000);

    res = dec.decode(buff + decoded_size, len - decoded_size);
    rep = res.value();
    decoded_size += res.size();
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == 13);
    REQUIRE(rep.type() == resp::ty_error);
    REQUIRE(rep.error() == "ERR errmsg");

    REQUIRE(decoded_size == len);
  }

  SECTION("incompleted")
  {
    char buff[32] = "+OK\r\n";
    resp::result res;

    res = dec.decode(buff, 3);
    REQUIRE(res == resp::incompleted);
    REQUIRE(res.size() == 3);

    res = dec.decode(buff + 3, 2);
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == 2);
  }

  SECTION("array")
  {
    char buff[32] = "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";
    resp::result res;
    resp::unique_value rep;

    res = dec.decode(buff, std::strlen(buff));
    rep = res.value();
    REQUIRE(res == resp::completed);
    REQUIRE(rep.type() == resp::ty_array);

    resp::unique_array<resp::unique_value> arr = rep.array();
    REQUIRE(arr.size() == 2);

    REQUIRE(arr[0].bulkstr() == "foo");
    REQUIRE(arr[1].bulkstr() == "bar");
  }

  SECTION("array incompleted")
  {
    char buff[32] = "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";
    size_t decoded_size = 0;
    size_t len = std::strlen(buff);
    resp::result res;
    resp::unique_value rep;

    res = dec.decode(buff, 2);
    REQUIRE(res == resp::incompleted);
    REQUIRE(res.size() == 2);

    res = dec.decode(buff + 2, 3);
    REQUIRE(res == resp::incompleted);
    REQUIRE(res.size() == 3);

    res = dec.decode(buff + 2 + 3, 5);
    REQUIRE(res == resp::incompleted);
    REQUIRE(res.size() == 5);

    res = dec.decode(buff + 2 + 3 + 5, 3);
    REQUIRE(res == resp::incompleted);
    REQUIRE(res.size() == 3);

    res = dec.decode(buff + 2 + 3 + 5 + 3, 9);
    REQUIRE(res == resp::completed);
    REQUIRE(res.size() == 9);
    rep = res.value();

    resp::unique_array<resp::unique_value> arr = rep.array();
    REQUIRE(arr.size() == 2);

    REQUIRE(arr[0].bulkstr() == "foo");
    REQUIRE(arr[1].bulkstr() == "bar");
  }

  SECTION("array recursion")
  {
    char buff[64] = "*2\r\n$3\r\nfoo\r\n*2\r\n$4\r\nfoo2\r\n$3\r\nbar\r\n";
    resp::result res;
    resp::unique_value rep;

    res = dec.decode(buff, std::strlen(buff));
    rep = res.value();
    REQUIRE(res == resp::completed);
    REQUIRE(rep.type() == resp::ty_array);

    resp::unique_array<resp::unique_value> arr = rep.array();
    REQUIRE(arr.size() == 2);

    REQUIRE(arr[0].bulkstr() == "foo");
    REQUIRE(arr[1].type() == resp::ty_array);

    resp::unique_array<resp::unique_value> const& subarr = arr[1].array();
    REQUIRE(subarr.size() == 2);
    REQUIRE(subarr[0].bulkstr() == "foo2");
    REQUIRE(subarr[1].bulkstr() == "bar");
  }
}
