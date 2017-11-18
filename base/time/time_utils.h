#ifndef _BASE_TIME_UTILS_H_
#define _BASE_TIME_UTILS_H_

#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

namespace base {

//millseconds ms  1000
//microseconds us 1000000
//nanoseconds  ns 1000000000
static const int64_t kNumMillisecsPerSec = INT64_C(1000);
static const int64_t kNumMicrosecsPerSec = INT64_C(1000000);
static const int64_t kNumNanosecsPerSec = INT64_C(1000000000);

static const int64_t kNumMicrosecsPerMillisec =
      kNumMicrosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMillisec =
      kNumNanosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMicrosec =
        kNumNanosecsPerSec / kNumMicrosecsPerSec;

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
