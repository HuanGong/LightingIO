// this is copy from a zhihui post, thanks for his implimention

#ifndef _BASE_SPIN_LOCK_H_H
#define  _BASE_SPIN_LOCK_H_H

#include <memory>
#include <unistd.h>
#include <atomic>
#include <x86intrin.h>
#include <base_micro.h>
namespace base {

class SpinLock {
public:
  SpinLock();
  void lock();
  bool try_lock();
  void unlock();
private:
  std::atomic<bool>  d_atomic_bool;
  DISALLOW_COPY_AND_ASSIGN(SpinLock);
};

//be carefor use is
class SpinLockGuard {
public:
  SpinLockGuard(SpinLock& lock);
  ~SpinLockGuard();
private:
  SpinLock& lock_;
  DISALLOW_COPY_AND_ASSIGN(SpinLockGuard);
};

}

#endif
