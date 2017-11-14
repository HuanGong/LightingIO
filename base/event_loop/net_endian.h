#ifndef LIGHTING_NET_ENDIAN_H
#define LIGHTING_NET_ENDIAN_H

#include <stdint.h>
#include <endian.h>

namespace net {
namespace endian {
// the inline assembler code makes type blur,
// so we disable warnings for a while.
#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

inline uint64_t HostToNetwork64(uint64_t host64) {
  return htobe64(host64);
}

inline uint32_t HostToNetwork32(uint32_t host32) {
  return htobe32(host32);
}

inline uint16_t HostToNetwork16(uint16_t host16) {
  return htobe16(host16);
}

inline uint64_t NetworkToHost64(uint64_t net64) {
  return be64toh(net64);
}

inline uint32_t NetworkToHost32(uint32_t net32) {
  return be32toh(net32);
}

inline uint16_t NetworkToHost16(uint16_t net16) {
  return be16toh(net16);
}

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic warning "-Wconversion"
#pragma GCC diagnostic warning "-Wold-style-cast"
#endif

}}
#endif  // LIGHTING_NET_ENDIAN_H
