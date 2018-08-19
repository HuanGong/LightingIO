///
/// main.cpp
/// 
/// Example for redis pipeline using.
/// 

#include <resp/all.hpp>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstring>

int main()
{
  /// Create encoder.
  resp::encoder<resp::buffer> enc;
  
  /// Create decoder.
  resp::decoder dec;
  
  /// User's buffers.
  std::vector<resp::buffer> buffers;
  
  /// Encode set and get commands.
  enc
    .begin(buffers)
      .cmd("SET").arg("key").arg("value").end()
      .cmd("GET").arg("key").end()
    .end();
  
  /// buffers == "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n"  
  assert(buffers.empty() == false);
  
  /// Decode set and get's reply.
  char reply[64] = "+OK\r\n$5\r\nvalue\r\n";
  size_t reply_len = std::strlen(reply);
  
  /// Decode result.
  resp::result res;
  resp::unique_value rep;
  size_t decoded_size = 0;
  
  /// One resp::decoder::decode, one reply.(if buffer decode complete)
  res = dec.decode(reply, reply_len);
  assert(res == resp::completed);
  rep = res.value();
  assert(rep.type() == resp::ty_string);
  assert(rep.string() == "OK");
  
  /// Set decoded size for next decode.
  decoded_size += res.size();
  
  /// The second decoding, note for buffer offset (using decoded_size).
  res = dec.decode(reply + decoded_size, reply_len - decoded_size);
  assert(res == resp::completed);
  rep = res.value();
  assert(rep.type() == resp::ty_bulkstr);
  assert(rep.bulkstr() == "value");
  
  return 0;
}
