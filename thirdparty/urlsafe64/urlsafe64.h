#ifndef __URLSAFE64_H__
#define __URLSAFE64_H__

#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define urlsafe64_get_encoded_size(siz) ((4 * (siz) + 2) / 3)

int urlsafe64_encode(const uint8_t *src, int src_len, char *dst);
int urlsafe64_decode(const char *src, int src_len, uint8_t *dst);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif//__URLSAFE64_H__
