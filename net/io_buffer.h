#ifndef _NET_IO_BUFFER_H_H
#define _NET_IO_BUFFER_H_H

#include <vector>
#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <string>
#include <string.h>

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

  inline int32_t CanReadSize() {
    return write_index_ - read_index_;
  }
  inline int32_t CanWriteSize() {
    return data_.size() - write_index_;
  }

  const uint8_t* FindCRLF();

  void WriteString(const std::string& str);
  void WriteRawData(const void* data, int32_t len);

  template <typename T> void Append(T data) {
    union _X {
      T _data;
      char _x;
    };
    uint32_t len = sizeof(data);
    EnsureWritableSize(len);
    ::memcpy(MutableWrite(), (uint8_t*) &data, len);
    Produce(len);
  }

  std::string AsString();

  void Consume(int32_t len);

  inline void Produce(int32_t len) {
    write_index_ += len;
  }
private:
  uint8_t* MutableRead();
  uint8_t* MutableWrite();

  int32_t read_index_;
  int32_t write_index_;
  std::vector<uint8_t> data_;
};

}
#endif
