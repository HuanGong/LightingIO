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
  static Timestamp AfterSecond(time_t sec);
  static Timestamp AfterMicroSecond(time_t us);

  time_t AsMillsecond();
  std::string ToString();

  bool operator==(const Timestamp& other) const;
  bool operator>(const Timestamp& other) const;
  bool operator<(const Timestamp& other) const;

  inline time_t Second() const {
    return tv_.tv_sec;
  }
  inline time_t MicroSecond() const {
    return tv_.tv_usec;
  }
private:
  timeval tv_;
};

}//end base
#endif
