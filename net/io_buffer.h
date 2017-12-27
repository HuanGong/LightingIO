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

  int32_t ReadFromSocketFd(int scoket, int *error);
  //IOBuffer Clone();

  bool EnsureWritableSize(int32_t len);
  bool HasALine();

  const uint8_t* GetRead();
  uint8_t* GetWrite();
  int32_t CanReadSize();
  int32_t CanWriteSize();

  const uint8_t* FindCRLF();

  void WriteString(const std::string str);
  void WriteRawData(const void* data, int32_t len);

  std::string AsString();

  void Consume(int32_t len);
  void Produce(int32_t len);
private:
  uint8_t* MutableRead();
  uint8_t* MutableWrite();

  int32_t read_index_;
  int32_t write_index_;
  std::vector<uint8_t> data_;
};

}
#endif
