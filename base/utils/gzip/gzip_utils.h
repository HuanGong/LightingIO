#include <string>
#include <sstream>
#include <stdexcept>

#include <string.h>
#include "zlib.h"

namespace base {

inline char* str_as_array(std::string* str) {
  return str->empty() ? NULL : &*str->begin();
}
inline const char* str_as_array(const std::string& str) {
  return str_as_array(const_cast<std::string*>(&str));
}

namespace Gzip {

//return zero when success, other for fails
int decompress_gzip(const std::string& str, std::string& out);
int decompress_deflate(const std::string& str, std::string& out);

//
int compress_gzip(const std::string& str, std::string& out, int compressionlevel = Z_BEST_COMPRESSION);
//std::string compress_gzip(const std::string& str, int compressionlevel = Z_BEST_COMPRESSION);
int compress_deflate(const std::string& str, std::string& out, int compressionlevel = Z_BEST_COMPRESSION);

}}//end base::Gzip
