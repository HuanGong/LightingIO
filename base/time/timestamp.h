#ifndef BASE_TIMESTAMP_H_H
#define BASE_TIMESTAMP_H_H

#include "time_utils.h"
#include <string>

namespace base {
//time ensure the us always less than 1000000
class Timestamp {
public:
  Timestamp(const timeval tv);
  Timestamp(time_t sec, time_t us);
  Timestamp(const Timestamp& other);

  static Timestamp Now();
  static Timestamp NSecondLater(time_t sec);
  static Timestamp NMillisecondLater(time_t ms);
  static Timestamp NMicrosecondLater(time_t us);

  int64_t AsMillsecond() const;
  int64_t AsMicroSecond() const;
  std::string ToString();

  bool operator!=(const Timestamp& other) const;
  bool operator==(const Timestamp& other) const;
  bool operator>(const Timestamp& other) const;
  bool operator<(const Timestamp& other) const;
  bool operator<=(const Timestamp& other) const;

private:
  int64_t microsecond_time_;
};

}//end base
#endif
