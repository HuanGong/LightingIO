#ifndef __CRC_H_2011__
#define __CRC_H_2011__

/**
 * @file crc.h
 * This file is ported from FFmpeg.
 */

#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

uint32_t crc8_atm(uint32_t start, const uint8_t *buf, uint32_t siz);
uint32_t crc16_ansi(uint32_t start, const uint8_t *buf, uint32_t siz);
uint32_t crc16_ccitt(uint32_t start, const uint8_t *buf, uint32_t siz);
uint32_t crc32_ieee(uint32_t start, const uint8_t *buf, uint32_t siz);
uint32_t crc32_ieee_le(uint32_t start, const uint8_t *buf, uint32_t siz);

uint32_t crc32_ieee_le_rev(uint32_t e, const uint8_t *buf, uint32_t siz);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* __CRC_H_2011__ */


