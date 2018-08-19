///
/// main.cpp
/// 
/// Example for reuse encode buffers.
/// 

#include <resp/all.hpp>
#include <vector>
#include <cassert>

int main()
{
  /// Create encoder.
  resp::encoder<resp::buffer> enc;
  
  /// User can create encode buffers before encode, and reuse it: 
  /// std::vector's memory not clear.
  std::vector<resp::buffer> buffers;
  
  /// Encoder use begin and end to reuse buffers.
  /// Use cmd to set redis command; arg set command's args. 
  enc
    .begin(buffers)
      .cmd("SET").arg("key").arg("value").end()
    .end();
  /// buffers == "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n"
  assert(buffers.empty() == false);
  
  /// Reuse buffers, buffers will be clear in enc.begin().
  enc
    .begin(buffers)
      .cmd("GET").arg("key").end()
    .end();
  /// buffers == "*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n"
  assert(buffers.empty() == false);
  
  return 0;
}
