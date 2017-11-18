#include "timestamp.h"
#include <string>
#include <glog/logging.h>

namespace base {

Timestamp::Timestamp(const timeval tv) {
  microsecond_time_ = (int64_t)tv.tv_sec * (int64_t)kNumMicrosecsPerSec + tv.tv_usec;
}
Timestamp::Timestamp(time_t sec, time_t us) {
  microsecond_time_ = sec * kNumMicrosecsPerSec + us;
}
Timestamp::Timestamp(const Timestamp& other) {
  microsecond_time_ =  other.microsecond_time_;
}

//static
Timestamp Timestamp::Now() {
  timeval tv;
  ::gettimeofday(&tv, NULL);
  return Timestamp(tv);
}

//static
Timestamp Timestamp::NSecondLater(time_t sec) {
  timeval tv;
  ::gettimeofday(&tv, NULL);
  tv.tv_sec += sec;
  return Timestamp(tv);
}

//static
Timestamp Timestamp::NMicrosecondLater(time_t us) {
  timeval tv;
  ::gettimeofday(&tv, NULL);
  tv.tv_usec += us;
  return Timestamp(tv);
}

//static
Timestamp Timestamp::NMillisecondLater(time_t ms) {
  return NMicrosecondLater(ms * kNumMicrosecsPerMillisec);
}

int64_t Timestamp::AsMicroSecond() const {
  return microsecond_time_;
}

int64_t Timestamp::AsMillsecond() const {
  return microsecond_time_ / kNumMicrosecsPerMillisec;
}

std::string Timestamp::ToString() {
  return std::to_string(microsecond_time_);
}

bool Timestamp::operator>(const Timestamp& other) const {
  return microsecond_time_ > other.microsecond_time_;
}

bool Timestamp::operator==(const Timestamp& other) const {
  return microsecond_time_ == other.microsecond_time_;
}

bool Timestamp::operator<(const Timestamp& other) const {
  return microsecond_time_ < other.microsecond_time_;
}
bool Timestamp::operator<=(const Timestamp& other) const {
  return microsecond_time_ <= other.microsecond_time_;
}

bool Timestamp::operator!=(const Timestamp& other) const {
  return microsecond_time_ != other.microsecond_time_;
}

}
