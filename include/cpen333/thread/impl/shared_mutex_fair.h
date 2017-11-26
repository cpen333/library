/**
 * @file
 * @brief Implementation of a mutex with shared access that tries to fairly balance priorities
 * between exclusive and shared use
 */

#ifndef CPEN333_THREAD_SHARED_MUTEX_FAIR_H
#define CPEN333_THREAD_SHARED_MUTEX_FAIR_H

#include <mutex>
#include <condition_variable>
#include "../../util.h"

namespace cpen333 {
namespace thread {

namespace impl {

/**
 * @brief A shared mutex implementation with balanced priorities
 *
 * A more fair shared mutex, access is granted in batches: 1 writer, batch of readers, 1 writer, batch of readers
 * Based on the alternating method described here:
 *     http://www.tools-of-computing.com/tc/CS/Monitors/AlternatingRW.htm
 */
class shared_mutex_fair {
 private:

  struct shared_data {
    size_t shared[2];   // readers or queued
    char this_batch;    // index within shared of current batch sharing access
    char exclusive;     // # exclusive access, 0 or 1
    size_t etotal;      // waiting and exclusive access
  };

  std::mutex mutex_;               // mutex for state access
  std::condition_variable econd_;  // exclusive condition
  shared_data state_;              // shared counter object

 public:

  /**
   * @brief Constructor, creates a fair shared mutex
   */
  shared_mutex_fair() :
      mutex_(),
      econd_(),
      state_() {

    // initialize storage
    state_.shared[0] = 0;
    state_.shared[1] = 0;
    state_.this_batch = 0;
    state_.exclusive = 0;
    state_.etotal = 0;
     
  }

  // disable copy/move constructors
 private:
  shared_mutex_fair(const shared_mutex_fair &) DELETE_METHOD;
  shared_mutex_fair(shared_mutex_fair &&) DELETE_METHOD;
  shared_mutex_fair &operator=(const shared_mutex_fair &) DELETE_METHOD;
  shared_mutex_fair &operator=(shared_mutex_fair &&) DELETE_METHOD;

 public:

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::lock_shared()
   */
  void lock_shared() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_.etotal == 0) {
      // let him pass
      ++(state_.shared[(size_t)(state_.this_batch)]);
    } else {
      // wait until I am told to go by end of exclusive lock
      size_t batch = 1-state_.this_batch;  // next batch
      ++(state_.shared[batch]);
      econd_.wait(lock, [&](){ return (size_t)(state_.this_batch) == batch; });
    }
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::try_lock_shared()
   */
  bool try_lock_shared() {
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    if (!lock.try_lock()) {
      return false;
    }

    if (state_.exclusive > 0) {
      return false;
    }
    return true;
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::unlock_shared()
   */
  void unlock_shared() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (--(state_.shared[(size_t)(state_.this_batch)]) == 0) {
      econd_.notify_all();     // tell others to check their conditions
    }
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::lock()
   */
  void lock() {
    std::unique_lock<std::mutex> lock(mutex_);
    // one more waiting
    ++(state_.etotal);
    // wait until there are no readers or writers
    econd_.wait(lock, [&](){ return state_.exclusive == 0 && state_.shared[(size_t)(state_.this_batch)] == 0; });
    state_.exclusive = 1;  // exclusive lock on
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::try_lock()
   */
  bool try_lock() {
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    if (!lock.try_lock()) {
      return false;
    }

    // check if we can go immediately
    if (state_.shared[(size_t)(state_.this_batch)]+state_.exclusive > 0) {
      return false;
    }

    // we got through
    ++(state_.etotal);
    state_.exclusive = 1;  // exclusive lock on
    return true;
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::unlock()
   */
  void unlock() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.exclusive = 0;
    --state_.etotal;
    // send through next batch and notify
    state_.this_batch = 1-state_.this_batch;
    econd_.notify_all();
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::try_lock_for()
   */
  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
    return try_lock_until(std::chrono::steady_clock::now() + timeout_duration);
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::try_lock_until()
   */
  template<class Clock, class Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration> &timeout_time) {
    // prevent readers from passing me, since they need this
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    if (!lock.try_lock_until(timeout_time)) {
      return false;
    }

    // one more waiting
    ++(state_.etotal);
    // wait until there are no readers or writers
    if (!econd_.wait_until(lock, timeout_time,
                           [&](){ return state_.shared[(size_t)(state_.this_batch)]+state_.exclusive == 0; })) {
      // timed out, undo wait
      --(state_.etotal);
      return false;
    }

    state_.exclusive = 1;  // exclusive lock on
    return true;
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::try_lock_shared_for()
   */
  template<class Rep, class Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
    return try_lock_shared_until(std::chrono::steady_clock::now() + timeout_duration);
  }

  /**
   * @copydoc cpen333::thread::impl::shared_mutex_exclusive::try_lock_shared_until()
   */
  template<class Clock, class Duration>
  bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration> &timeout_time) {
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock); // do not try yet
    if (!lock.try_lock_until(timeout_time)) {
      return false;
    }

    if (state_.etotal == 0) {
      // let him pass
      ++(state_.shared[(size_t)(state_.this_batch)]);
    } else {
      // wait until I am told to go by end of exclusive lock
      size_t batch = 1-state_.this_batch;  // next batch
      ++(state_.shared[batch]);
      if (!econd_.wait_until(lock, timeout_time,
                             [&](){ return state_.this_batch == batch; })) {
        // undo queued reader
        --(state_.shared[batch]);
        return false;
      }
    }
    return true;
  }

};

} // impl

/**
 * @brief Alias for default shared mutex with fair priority
 */
typedef impl::shared_mutex_fair shared_mutex_fair;
/**
 * @brief Alias for default shared timed mutex with fair priority
 */
typedef impl::shared_mutex_fair shared_timed_mutex_fair;

} // process
} // cpen333

namespace std {

/**
 * @brief Shared mutex replacement
 */
typedef cpen333::thread::shared_mutex_fair shared_mutex;

/**
 * @brief Shared timed mutex replacement
 */
typedef cpen333::thread::shared_mutex_fair shared_timed_mutex;

}

#endif //CPEN333_PROCESS_SHARED_MUTEX_FAIR_H
