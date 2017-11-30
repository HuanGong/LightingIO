#ifndef _NET_IO_BUFFER_H_H
#define _NET_IO_BUFFER_H_H

#include <vector>
#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <string>

namespace net {
#define kDefaultInitialSize 4096

class IOBuffer {
public:
  IOBuffer(int32_t size = kDefaultInitialSize);
  IOBuffer(const IOBuffer&& r);
  ~IOBuffer();

  //IOBuffer Clone();

  bool EnsureWritableSize(int32_t len);

  uint8_t* GetRead();
  uint8_t* GetWrite();
  int32_t CanReadSize();
  int32_t CanWriteSize();

  void WriteString(const std::string str);
  void WriteRawData(const uint8_t* data, int32_t len);

  std::string AsString();

private:
  void consume(int32_t len);
  void produce(int32_t len);

  int32_t read_index_;
  int32_t write_index_;
  std::vector<uint8_t> data_;
};

}
#endif
