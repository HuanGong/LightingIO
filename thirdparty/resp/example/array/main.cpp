///
/// main.cpp
/// 
/// Example for decoding array.
/// 

#include <resp/all.hpp>
#include <cassert>
#include <cstdlib>
#include <cstring>

int main()
{
  /// Create decoder.
  resp::decoder dec;
  
  /// Decode a array.
  {
    char buff[32] = "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";
    resp::result res;
    resp::unique_value rep;

    res = dec.decode(buff, std::strlen(buff));
    rep = res.value();
    assert(res == resp::completed);
    assert(rep.type() == resp::ty_array);

    resp::unique_array<resp::unique_value> arr = rep.array();
    assert(arr.size() == 2);

    assert(arr[0].bulkstr() == "foo");
    assert(arr[1].bulkstr() == "bar");
  }
  
  /// Decode a nest array.
  {
    char buff[64] = "*2\r\n$3\r\nfoo\r\n*2\r\n$4\r\nfoo2\r\n$3\r\nbar\r\n";
    resp::result res;
    resp::unique_value rep;

    res = dec.decode(buff, std::strlen(buff));
    rep = res.value();
    assert(res == resp::completed);
    assert(rep.type() == resp::ty_array);

    resp::unique_array<resp::unique_value> arr = rep.array();
    assert(arr.size() == 2);

    assert(arr[0].bulkstr() == "foo");
    assert(arr[1].type() == resp::ty_array);

    resp::unique_array<resp::unique_value> const& subarr = arr[1].array();
    assert(subarr.size() == 2);
    assert(subarr[0].bulkstr() == "foo2");
    assert(subarr[1].bulkstr() == "bar");
  }
  
  return 0;
}
