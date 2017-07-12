#ifndef NS_CONVERTOR_H_H
#define NS_CONVERTOR_H_H

#include <cstdint>
#include <cstddef>

namespace base {
namespace utils {

// POSIX doesn't define any async-signal safe function for converting
// an integer to ASCII. We'll have to define our own version.
// itoa_r() converts a (signed) integer to ASCII. It returns "buf", if the
// conversion was successful or NULL otherwise. It never writes more than "sz"
// bytes. Output will be truncated as needed, and a NUL character is always
// appended.
char *itoa_r(intptr_t i, char *buf, size_t sz, int base, size_t padding);


}} //end namespace
#endif
