///
/// main.cpp
/// 
/// Example for decoding incompleted buffer.
/// 

#include <resp/all.hpp>
#include <cassert>
#include <cstdlib>
#include <cstring>

int main()
{
  /// Create decoder.
  resp::decoder dec;
  
  /// Decode some cmd reply.
  char reply[64] = "+OK\r\n";
  size_t reply_len = std::strlen(reply);
  
  char buff[32] = "+OK\r\n";
  resp::result res;

  /// Decode a incompleted buffer.
  res = dec.decode(buff, 3);
  assert(res == resp::incompleted);
  assert(res.size() == 3);

  /// Decode the rest.
  res = dec.decode(buff + 3, 2);
  assert(res == resp::completed);
  assert(res.size() == 2);
  
  resp::unique_value rep = res.value();
  assert(rep.type() == resp::ty_string);
  assert(rep.string() == "OK");
  
  return 0;
}
