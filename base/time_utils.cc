
#include "time_utils.h"

//millseconds ms  1000
//microseconds us 1000000
//nanoseconds  ns 1000000000
static const int64_t kNumMillisecsPerSec = INT64_C(1000);
static const int64_t kNumMicrosecsPerSec = INT64_C(1000000);
static const int64_t kNumNanosecsPerSec = INT64_C(1000000000);

//
static const int64_t kNumMicrosecsPerMillisec =
      kNumMicrosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMillisec =
      kNumNanosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMicrosec =
        kNumNanosecsPerSec / kNumMicrosecsPerSec;

namespace base {

int64_t SystemTimeNanos() {
  int64_t ticks;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ticks = kNumNanosecsPerSec * static_cast<int64_t>(ts.tv_sec) + static_cast<int64_t>(ts.tv_nsec);
  return ticks;
}
//ms
int64_t SystemTimeMillisecs() {
  struct timeval tv;
  ::gettimeofday(&tv, NULL);
  return tv.tv_usec/kNumMicrosecsPerMillisec + tv.tv_sec*kNumMillisecsPerSec;
}

//us
uint32_t time_us() {
  return static_cast<uint32_t>(SystemTimeNanos() / kNumNanosecsPerMicrosec);
}
//ms
uint32_t time_ms() {
  return static_cast<uint32_t>(SystemTimeNanos() / kNumNanosecsPerMillisec);
}

inline uint32_t delta_ms(const uint32_t before_ms) {
  return time_ms() - before_ms;
}

inline uint32_t delta_us(const uint32_t before_us) {
  return time_us() - before_us;
}

struct timeval ms_to_timeval(uint32_t ms) {
  timeval tv;
  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  return tv;
}

}//end namespace
