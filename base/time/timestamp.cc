#include "timestamp.h"
#include <string>

namespace base {

Timestamp::Timestamp(const timeval tv)
  : tv_(tv) {
}
Timestamp::Timestamp(time_t sec, time_t us) {
  tv_.tv_sec = sec;
  tv_.tv_usec = us;
}
Timestamp::Timestamp(const Timestamp& other) {
  tv_ = other.tv_;
}

//static
Timestamp Timestamp::Now() {
  timeval tv;
  ::gettimeofday(&tv, NULL);
  return Timestamp(tv);
}

//static
Timestamp Timestamp::AfterSecond(time_t sec) {
  timeval tv;
  ::gettimeofday(&tv, NULL);
  tv.tv_sec += sec;
  return Timestamp(tv);
}

//static
Timestamp Timestamp::AfterMicroSecond(time_t us) {
  timeval tv;
  ::gettimeofday(&tv, NULL);

  time_t sum_usec = tv.tv_usec + us;

  tv.tv_usec = sum_usec % 1000000;
  tv.tv_sec += (sum_usec - tv.tv_usec) / 1000000;

  return Timestamp(tv);
}

time_t Timestamp::AsMillsecond() {
  return (tv_.tv_sec * 1000) + (tv_.tv_usec / 1000);
}

std::string Timestamp::ToString() {
  std::string timestamp = std::to_string(tv_.tv_sec);
  timestamp += " second";
  timestamp += std::to_string(tv_.tv_usec);
  timestamp += " microsecond";
  return timestamp;
}

bool Timestamp::operator>(const Timestamp& other) const {
  if (Second() == other.Second()) {
    return MicroSecond() > other.MicroSecond();
  }
  return Second() > other.MicroSecond();
}

bool Timestamp::operator==(const Timestamp& other) const {
  return Second() == other.Second();
}

bool Timestamp::operator<(const Timestamp& other) const {
  return ! ((*this) > other);
}

}
