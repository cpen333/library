/**
 * @file
 * @brief Implementation of a mutex with shared access that gives priority to exclusive use
 * (write-priority)
 */
#ifndef CPEN333_THREAD_SHARED_MUTEX_EXCLUSIVE_H
#define CPEN333_THREAD_SHARED_MUTEX_EXCLUSIVE_H

#include <mutex>
#include <condition_variable>
#include "../semaphore.h"
#include "../condition.h"

namespace cpen333 {
namespace thread {

namespace impl {

/**
 * @brief A write-preferring inter-process shared mutex implementation
 *
 * IA shared mutex implementation based on the mutex/semaphore pattern.  Gives priority to
 * exclusive (write) access.
 *
 * See https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock for details
 */
class shared_mutex_exclusive {
 private:

  std::mutex mutex_;                         // mutex for shared access
  cpen333::thread::semaphore global_;        // global semaphore
  size_t shared_count_;
  size_t exclusive_count_;
  std::mutex exclusive_;                     // protect writes to the "exclusive" field, CANNOT WAIT HERE
  cpen333::thread::condition cond_;          // condition of no writers

 public:

  /**
   * Constructor, creates a write-preferring shared mutex
   */
  shared_mutex_exclusive() :
      mutex_(),
      global_(1),   // gate opened
      shared_count_(0),
      exclusive_count_(0),
      exclusive_(),
      cond_(true)  // gate starts opened
  {}

 private:
  // disable copy/move constructors
  shared_mutex_exclusive(const shared_mutex_exclusive &) DELETE_METHOD;
  shared_mutex_exclusive(shared_mutex_exclusive &&) DELETE_METHOD;
  shared_mutex_exclusive &operator=(const shared_mutex_exclusive &) DELETE_METHOD;
  shared_mutex_exclusive &operator=(shared_mutex_exclusive &&) DELETE_METHOD;

 public:

  /**
   * @brief Lock the mutex in shared access mode
   *
   * Multiple threads can lock in shared mode concurrently, allowing simultaneous access.  This method
   * will block if the mutex is currently locked in exclusive mode.
   */
  void lock_shared() {

    cond_.wait();           // wait until no exclusive access

    // increment number of waiting readers
    std::lock_guard<std::mutex> lock(mutex_);
    if (++shared_count_ == 1) {
      global_.wait();       // "lock" semaphore preventing write access, could be waiting here for writer to finish
    }
  }

  /**
   * @brief Tries to lock the mutex in shared access mode
   *
   * Multiple threads can lock in shared mode concurrently, allowing simultaneous access.  This method returns
   * immediately.
   *
   * @return true if successfully locked, false if mutex is currently locked in exclusive access mode
   */
  bool try_lock_shared() {
    if (!cond_.wait_for(std::chrono::milliseconds(0))) {
      return false;
    }

    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    if (!lock.try_lock()) {
      return false;
    }

    if (shared_count_ == 0) {
      bool success = global_.try_wait();  // "lock" semaphore preventing writes
      // only increment if successful
      if (!success) {
        return false;
      }
      shared_count_ = 1;
    } else {
      ++shared_count_;
    }
    return true;
  }

  /**
   * @brief Unlocks one instance of shared access
   *
   * The mutex will continue to remain locked in shared access mode until all shared locks are unlocked.
   */
  void unlock_shared() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (--shared_count_ == 0) {
      global_.notify(); // "unlock" semaphore
    }
  }

  /**
   * @brief Locks the mutex in exclusive access mode
   *
   * Only one thread can lock in exclusive access mode.  This method
   * will block if the mutex is currently locked in either shared or exclusive mode.
   */
  void lock() {
    // next in line
    {
      std::lock_guard<std::mutex> lock(mutex_);
      // if only one writer, block all future readers
      std::lock_guard<std::mutex> ex(exclusive_);
      if (++exclusive_count_ == 1) {
        cond_.reset();
      }
    }
    global_.wait(); // lock semaphore
  }

  /**
   * @brief Tries to lock the mutex in exclusive access mode
   *
   * Only one thread can lock in exclusive access mode.  This method returns immediately.
   *
   * @return true if successfully locked, false if already locked in either shared or exclusive access mode
   */
  bool try_lock() {

    // next in line
    {
      std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
      if (!lock.try_lock()) {
        return false; // someone is currently locked
      }

      // try holding both locks at once immediately
      if (!global_.try_wait()) {
        return false;
      }

      // I now hold both locks, which means there are no current readers or writers
      std::lock_guard<std::mutex> ex(exclusive_);
      exclusive_count_ = 1;
      cond_.reset();   // prevent future readers
    }

    return true;
  }

  /**
   * @brief Unlocks the exclusively-locked mutex
   */
  void unlock() {

    global_.notify(); // unlock semaphore  // allow next reader/writer waiting

    std::lock_guard<std::mutex> lock(mutex_);   // access to data
    std::lock_guard<std::mutex> ex(exclusive_);
    if (--exclusive_count_ == 0) {
      cond_.notify();  // open gate for readers
    }
  }

  /**
   * @brief Try to exclusively lock the mutex, with a relative timeout
   *
   * Tries to lock the mutex in exclusive-access (write) mode, returns if the mutex
   * has been unavailable for the specified timeout duration
   *
   * @tparam Rep duration representation
   * @tparam Period duration period
   * @param timeout_duration timeout duration
   * @return true if locked successfully
   */
  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
    return try_lock_until(std::chrono::steady_clock::now() + timeout_duration);
  }

  /**
   * @brief Try to exclusively lock the mutex, within an absolute timeout period
   *
   * Tries to lock the mutex in exclusive-access (write) mode, returns if the mutex has been
   * unavailable until specified time point has been reached
   *
   * @tparam Clock clock representation
   * @tparam Duration time
   * @param timeout_time time of timeout
   * @return true if locked successfully
   */
  template<class Clock, class Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration> &timeout_time) {

    // next in line
    {
      std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
      if (!lock.try_lock_until(timeout_time)) {
        return false;
      }

      // notify the line that there is an exclusive lock waiting
      std::lock_guard<std::mutex> ex(exclusive_);  // fast lock, doesn't protect any waits
      if (++exclusive_count_ == 1) {
        cond_.reset();
      }
    }

    if (!global_.wait_until(timeout_time)) {
      // quickly remove exclusive waiter, allowing others to proceed
      std::lock_guard<std::mutex> ex(exclusive_);
      if (--exclusive_count_ == 0) {
        cond_.notify();  // open gate for readers
      }
    }
    return true;
  }

  /**
   * @brief Try to lock the mutex in shared mode, with a relative timeout
   *
   * Tries to lock the mutex in shared-access (read) mode, returns if the mutex has been unavailable
   * for the specified timeout duration
   *
   * @tparam Rep duration representation
   * @tparam Period duration period
   * @param timeout_duration timeout duration
   * @return true if locked successfully
   */
  template<class Rep, class Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
    return try_lock_shared_until(std::chrono::steady_clock::now() + timeout_duration);
  }

  /**
   * @brief Try to lock the mutex in shared mode, with absolute timeout
   *
   * Tries to lock the mutex in shared-access (read) mode, returns if the mutex has been unavailable until
   * specified time point has been reached
   *
   * @tparam Clock clock representation
   * @tparam Duration time
   * @param timeout_time time of timeout
   * @return true if locked successfully
   */
  template<class Clock, class Duration>
  bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration> &timeout_time) {

    if (!cond_.wait_until(timeout_time)) {
      return false;
    }
    // no current exclusives waiting

    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    if (!lock.try_lock_until(timeout_time)) {
      return false;
    }

    if (shared_count_ == 0) {
      bool success = global_.wait_until(timeout_time);  // "lock" semaphore preventing writes
      // only increment if successful
      if (!success) {
        return false;
      }
      shared_count_ = 1;
    } else {
      ++shared_count_;
    }
    return true;
  }
};

} // impl

/**
 * @brief Alias for default shared mutex with exclusive (write) priority
 */
typedef impl::shared_mutex_exclusive shared_mutex_exclusive;

/**
 * @brief Alias for default shared timed mutex with exclusive (write) priority
 */
typedef impl::shared_mutex_exclusive shared_timed_mutex_exclusive;

} // thread
} // cpen333

#endif //CPEN333_THREAD_SHARED_MUTEX_EXCLUSIVE_H
