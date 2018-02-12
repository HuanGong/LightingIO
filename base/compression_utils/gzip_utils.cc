#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string.h>
#include "zlib.h"
#include "gzip_utils.h"
#include <iostream>

using std::string;

namespace base {

namespace Gzip {

#define INITSTRINGSIZE 16384

#define MOD_GZIP_ZLIB_WINDOWSIZE 15
#define MOD_GZIP_ZLIB_CFACTOR    9
#define MOD_GZIP_ZLIB_BSIZE      8096

int compress_gzip(const std::string& str, std::string& outstring,  int compressionlevel) {
  z_stream zs;                        // z_stream is zlib's control structure
  memset(&zs, 0, sizeof(zs));

  if (deflateInit2(&zs,
                   compressionlevel,
                   Z_DEFLATED,
                   MOD_GZIP_ZLIB_WINDOWSIZE + 16,
                   MOD_GZIP_ZLIB_CFACTOR,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    return -1;
  }

  zs.next_in = (Bytef*)str.data();
  zs.avail_in = str.size();           // set the z_stream's input

  int ret;
  outstring.resize(INITSTRINGSIZE, '\0');

  do {
    char* outbuffer = str_as_array(&outstring);
    if (NULL == outbuffer) return -1;

    zs.next_in = (Bytef*)(str.data() + zs.total_in);
    zs.avail_in= str.size() - zs.total_in;
    zs.next_out = (Bytef*)(outbuffer + zs.total_out);
    zs.avail_out = outstring.size() - zs.total_out;

    ret = deflate(&zs, Z_FINISH);
    if (ret == Z_STREAM_END) {
      break;
    }

    if (outstring.size() == zs.total_out) {
      outstring.resize(outstring.size()*2);
    }
  } while (ret == Z_OK);

  if (Z_OK != deflateEnd(&zs)) {
    return -1;
  }

  if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
    return -1;
  }
  outstring.resize(zs.total_out);
  return 0;
}

int compress_deflate(const std::string& str, std::string& outstring, int compressionlevel) {
  z_stream zs;                        // z_stream is zlib's control structure
  memset(&zs, 0, sizeof(zs));

  if (deflateInit(&zs, compressionlevel) != Z_OK) {
    return -1;
  }

  zs.next_in = (Bytef*)str.data();
  zs.avail_in = str.size();           // set the z_stream's input

  int ret;
  outstring.resize(INITSTRINGSIZE, '\0');

  do {
    char* outbuffer = str_as_array(&outstring);
    if (NULL == outbuffer) return -1;

    zs.next_in = (Bytef*)(str.data() + zs.total_in);
    zs.avail_in = str.size() - zs.total_in;
    zs.next_out = (Bytef*)(outbuffer + zs.total_out);
    zs.avail_out = outstring.size() - zs.total_out;

    ret = deflate(&zs, Z_FINISH);
    if (ret == Z_STREAM_END) {
      break;
    }

    if (outstring.size() == zs.total_out) {
      outstring.resize(outstring.size()*2);
    }
  } while (ret == Z_OK);

  if (Z_OK != deflateEnd(&zs)) {
    return -1;
  }

  if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
    return -1;
  }
  outstring.resize(zs.total_out);
  return 0;
}

int decompress_deflate(const std::string& str, std::string& outstring) {
  z_stream zs;                        // z_stream is zlib's control structure
  memset(&zs, 0, sizeof(zs));

  if (inflateInit(&zs) != Z_OK) {
    //throw(std::runtime_error("inflateInit failed while decompressing."));
    return -1;
  }

  zs.next_in = (Bytef*)str.data();
  zs.avail_in = str.size();

  outstring.resize(INITSTRINGSIZE, '\0'); //8kb

  int ret;

  // get the decompressed bytes blockwise using repeated calls to inflate
  do {
    char* outbuffer = str_as_array(&outstring); //must ensure outstring.size > 0
    if (outbuffer == NULL) return -1;

    zs.next_in = (Bytef*)(str.data() + zs.total_in);
    zs.avail_in = str.size() - zs.total_in;
    zs.next_out = (Bytef*)(outbuffer + zs.total_out);
    zs.avail_out = outstring.size() - zs.total_out;

    ret = inflate(&zs, 0);
    if (ret == Z_STREAM_END) {
      break;
    }
    if (outstring.size() == zs.total_out) {
      outstring.resize(outstring.size() * 2);
    }
  } while (ret == Z_OK);

  if (Z_OK != inflateEnd(&zs)) {
    return -1;
  }

  if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
    return -ret;
  }

  outstring.resize(zs.total_out);
  return 0;
}

int decompress_gzip(const std::string& str, std::string& outstring) {
  z_stream zs;                        // z_stream is zlib's control structure
  memset(&zs, 0, sizeof(zs));

  if (inflateInit2(&zs, MOD_GZIP_ZLIB_WINDOWSIZE + 16) != Z_OK) {
    //throw(std::runtime_error("inflateInit failed while decompressing."));
    std::cout << __FUNCTION__ << " return -1;";
    return -1;
  }
  outstring.resize(INITSTRINGSIZE, '\0');
  zs.next_in = (Bytef*)str.data();
  zs.avail_in = str.size();

  int ret;

  // get the decompressed bytes blockwise using repeated calls to inflate
  do {
    char* outbuffer = str_as_array(&outstring); //must ensure outstring.size > 0
    if (outbuffer == NULL) return -1;

    zs.next_in = (Bytef*)(str.data() + zs.total_in);
    zs.avail_in = str.size() - zs.total_in;

    zs.next_out = (Bytef*)(outbuffer + zs.total_out);
    zs.avail_out = outstring.size() - zs.total_out;

    ret = inflate(&zs, 0);
    if (ret == Z_STREAM_END) {
      break;
    }

    if (outstring.size() == zs.total_out) {
      outstring.resize(outstring.size() * 2);
    }
  } while (ret == Z_OK);

  if (Z_OK != inflateEnd(&zs)) {
    std::cout << __FUNCTION__ << " return -1;";
    return -1;
  }

  if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
    //throw(std::runtime_error(oss.str()));
    std::cout << __FUNCTION__ << " return -1;";
    return -1;
  }
  return 0;
}

}}//end base::Gzip

#if 0
// Compile: g++ -std=c++11 % -lz
// Run: ./a.out
#include <iostream>
int main() {
    std::string s = "Hello, 1234567890qwertyuiop[]asdfghjkl;'zxcvbnm,./";
    std::cout << "input: " << s << std::endl;

    std::string zipstr;
    base::Gzip::compress_gzip(s, zipstr);
    std::cout << "gzip compressed: " << zipstr << std::endl;;

    std::string out;
    std::cout << "gzip decompressed: " << base::Gzip::decompress_gzip(zipstr, out) << std::endl;;
    std::cout << "after  gzip decompress:" << out << std::endl;

    std::string flatestr;
    base::Gzip::compress_deflate(s, flatestr);
    std::cout << "flate compressed: " << flatestr << std::endl;;

    std::cout << "flate decompressed: " << base::Gzip::decompress_deflate(flatestr, out) << std::endl;;
    std::cout << "after falte decompress:" << out << std::endl;
}
#endif
