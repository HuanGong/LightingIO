#include "io_buffer.h"
#include <vector>
#include "glog/logging.h"

#include <unistd.h>
#include <termios.h>

#include <fcntl.h>
#include <cerrno>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <algorithm>

static const char kCRLF[] = "\r\n";
static const int32_t kWarningBufferSize = 64 * 1024 * 1024;

namespace net {

IOBuffer::IOBuffer(int32_t init_size) :
  read_index_(0),
  write_index_(0),
  data_(init_size) {
}

IOBuffer::IOBuffer(const IOBuffer&& r) :
  read_index_(r.read_index_),
  write_index_(r.write_index_),
  data_(std::move(r.data_)) {
}

IOBuffer::~IOBuffer() {
  data_.clear();
  read_index_ = 0;
  write_index_ = 0;
}

int32_t IOBuffer::ReadFromSocketFd(int socket, int *error) {
  int32_t bytes_can_read = -1;

  ::ioctl(socket, FIONREAD, &bytes_can_read);
  if (bytes_can_read == -1) {
    *error = errno;
    return -1;
  }

  if (bytes_can_read > 0) {
    EnsureWritableSize(bytes_can_read);
  }

  CHECK(CanWriteSize() >= bytes_can_read);

  struct iovec vec;
  vec.iov_base = GetWrite();
  vec.iov_len = CanWriteSize();

  const ssize_t bytes_read = ::readv(socket, &vec, 1);

  if (bytes_read > 0) {
    Produce(bytes_read);
  } else if (bytes_read < 0) {
    *error= errno;
  }
  return bytes_read;
}


bool IOBuffer::EnsureWritableSize(int32_t len) {
  if (data_.size() - write_index_ >= len) {
    return true;
  }

  if (CanWriteSize() + read_index_ >= len)  {

    int32_t data_size = CanReadSize();
    std::copy(data_.begin() + read_index_,
              data_.begin() + write_index_,
              data_.begin());
    read_index_ = 0;
    write_index_ = data_size;
    LOG_IF(INFO, data_.size() > kWarningBufferSize) << "Buffer Size > 32M";
    CHECK(CanWriteSize() >= len);
  } else {
    //TODO:consider move data too in some case
    data_.resize(write_index_ + len);
  }
  return true;
}

uint8_t* IOBuffer::MutableRead() {
  return &data_[read_index_];
}
uint8_t* IOBuffer::MutableWrite() {
  return &data_[write_index_];
}

const uint8_t* IOBuffer::GetRead() {
  return &data_[read_index_];
}
uint8_t* IOBuffer::GetWrite() {
  return &data_[write_index_];
}
int32_t IOBuffer::CanReadSize() {
  return write_index_ - read_index_;
}
inline int32_t IOBuffer::CanWriteSize() {
  return data_.size() - write_index_;
}

void IOBuffer::WriteString(const std::string& str) {
  EnsureWritableSize(str.size());
  memcpy(&data_[write_index_], str.data(), str.size());
  Produce(str.size());
}

void IOBuffer::WriteRawData(const void* data, int32_t len) {
  EnsureWritableSize(len);
  memcpy(&data_[write_index_], data, len);
  Produce(len);
}

std::string IOBuffer::AsString() {
  std::string str(data_.begin() + read_index_, data_.begin() + write_index_);
  return std::move(str);
}

void IOBuffer::Consume(int32_t len) {
  read_index_ += std::min(len, CanReadSize());
  if (read_index_ == write_index_) {
    read_index_ = write_index_ = 0;
  }
}

void IOBuffer::Produce(int32_t len) {
  write_index_ += len;
}

bool IOBuffer::HasALine() {
  return NULL !=  memchr(GetRead(), '\n', CanReadSize());
}

const uint8_t* IOBuffer::FindCRLF() {
  if (!CanReadSize()) {
    return NULL;
  }
  const uint8_t* res = std::search(MutableRead(), MutableWrite(), kCRLF, kCRLF+2);
  return res == GetWrite() ? NULL : res;
}


}
