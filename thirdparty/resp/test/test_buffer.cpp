///
/// test resp buffer
///

#define CATCH_CONFIG_NO_CPP11
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <resp/buffer.hpp>

TEST_CASE("resp buffer", "[buffer]")
{
  struct functor
  {
    void operator()(resp::buffer const& buf)
    {
      std::string str(buf.data(), buf.size());
      REQUIRE(str == "My test data");
    }
  };

  SECTION("create")
  {
    functor f;
    f("My test data");
    std::string str("My test data");
    f(str);

    resp::buffer buf(str.data(), str.size());
    REQUIRE(buf.data() == str.data());
    REQUIRE(buf.size() == str.size());
  }

  SECTION("append")
  {
    std::string str("My test data;");
    resp::buffer buf(str);
    REQUIRE(buf.is_ref());
    REQUIRE(buf.data() == str.data());
    REQUIRE(buf.size() == str.size());

    buf.append("append1;");
    REQUIRE(!buf.is_ref());
    REQUIRE(buf.data() != str.data());
    REQUIRE(buf.size() == str.size() + 8);

    buf.append("append2;");
    REQUIRE(!buf.is_ref());
    REQUIRE(buf.data() != str.data());
    REQUIRE(buf.size() == str.size() + 8 + 8);

    std::string finally(buf.data(), buf.size());
    REQUIRE(finally == "My test data;append1;append2;");

    buf.clear();
    REQUIRE(!buf.is_ref());
    REQUIRE(buf.size() == 0);

    buf.append("new data;");
    REQUIRE(!buf.is_ref());
    REQUIRE(buf.size() == 9);

    buf.append("append3;");
    REQUIRE(!buf.is_ref());
    REQUIRE(buf.size() == 9 + 8);

    finally.assign(buf.data(), buf.size());
    REQUIRE(finally == "new data;append3;");
  }

  SECTION("large buffer")
  {
    std::string str(RESP_SMALL_BUFFER_SIZE, 'a');
    resp::buffer buf(str);
    REQUIRE(buf.is_ref());
    REQUIRE(buf.data() == str.data());
    REQUIRE(buf.size() == str.size());

    buf.append("append1;");
    REQUIRE(buf.is_large());
    REQUIRE(buf.data() != str.data());
    REQUIRE(buf.size() == str.size() + 8);
  }
}
