///
/// test resp encoder
///

#define CATCH_CONFIG_NO_CPP11
#include <catch.hpp>
#include <resp/encoder.hpp>
#include <resp/buffer.hpp>

TEST_CASE("resp encoder", "[encoder]")
{
  resp::encoder<resp::buffer> enc;
  SECTION("default")
  {
    std::vector<resp::buffer> buffers = enc.encode("SET", "key", "value");
    REQUIRE(buffers.size() == 1);
    std::string query(buffers.front().data(), buffers.front().size());
    REQUIRE(query == "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n");
  }

  SECTION("large")
  {
    std::string value(RESP_LARGE_BUFFER_SIZE+1, 'a');
    std::vector<resp::buffer> buffers = enc.encode("SET", "key", value);
    REQUIRE(buffers.size() == 3);

    std::string query;

    query.assign(buffers[0].data(), buffers[0].size());
    REQUIRE(query == "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$1025\r\n");

    query.assign(buffers[1].data(), buffers[1].size());
    REQUIRE(buffers[1].is_ref());
    REQUIRE(query == value);
    REQUIRE(buffers[1].data() == value.data());

    query.assign(buffers[2].data(), buffers[2].size());
    REQUIRE(query == "\r\n");
  }

  SECTION("external buffers, command and pipelining")
  {
    std::vector<resp::buffer> buffers;
    enc
      .begin(buffers)
        .cmd("SET").arg("key").arg("value").end()
        .cmd("GET").arg("key").end()
      .end();
    REQUIRE(buffers.size() == 1);
    REQUIRE(buffers[0].is_small());
    std::string query;

    query.assign(buffers[0].data(), buffers[0].size());
    REQUIRE(query == "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n");

    enc
      .begin(buffers)
        .cmd("HMSET").arg("hkey").arg("field1").arg("value1").arg("field2").arg("value2").end()
        .cmd("HMGET").arg("hkey").arg("field1").arg("field2").end()
      .end();

    REQUIRE(buffers.size() == 1);
    REQUIRE(buffers[0].is_large());

    query.assign(buffers[0].data(), buffers[0].size());
    REQUIRE(query ==
      "*6\r\n$5\r\nHMSET\r\n$4\r\nhkey\r\n$6\r\nfield1\r\n$6\r\nvalue1\r\n$6\r\nfield2\r\n$6\r\nvalue2\r\n*4\r\n$5\r\nHMGET\r\n$4\r\nhkey\r\n$6\r\nfield1\r\n$6\r\nfield2\r\n"
      );
  }
}
