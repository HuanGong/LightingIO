
#include <thread>
#include "spin_lock.h"

namespace base {

SpinLock::SpinLock( void ) {
  d_atomic_bool.store(false, std::memory_order_relaxed);
}

//for less cpu consume
void SpinLock::lock( void ) {
  while( d_atomic_bool.exchange( true, std::memory_order_acquire )) {
    while( 1 )  {
      _mm_pause();
      if (!d_atomic_bool.load( std::memory_order_relaxed ))
        break;
      std::this_thread::yield();
      if( !d_atomic_bool.load( std::memory_order_relaxed ) )
        break;
      continue;
    }
    continue;
  }
}

bool SpinLock::try_lock( void ) {
  return  !d_atomic_bool.exchange( true, std::memory_order_acquire );
}

void SpinLock::unlock( void ) {
  d_atomic_bool.store(false, std::memory_order_release);  // 设置为false
}


SpinLockGuard::SpinLockGuard(SpinLock& lock)
  : lock_(lock) {
  lock_.lock();
}

SpinLockGuard::~SpinLockGuard() {
  lock_.unlock();
}

}//end base::

