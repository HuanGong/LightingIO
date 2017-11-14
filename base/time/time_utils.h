#ifndef _BASE_TIME_UTILS_H_
#define _BASE_TIME_UTILS_H_

#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

namespace base {

int64_t SystemTimeNanos();
//ms
int64_t SystemTimeMillisecs();
//us
uint32_t time_us();
//ms
uint32_t time_ms();

uint32_t delta_ms(const uint32_t before_ms);

uint32_t delta_us(const uint32_t before_us);

struct timeval ms_to_timeval(uint32_t ms);

}
#endif
