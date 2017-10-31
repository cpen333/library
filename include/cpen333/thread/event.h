/**
 * @file
 * @brief Event synchronization primitive
 */
#ifndef CPEN333_PROCESS_EVENT_H
#define CPEN333_PROCESS_EVENT_H

#include <chrono>
#include <mutex>
#include <condition_variable>
#include "semaphore.h"
#include "../util.h"

namespace cpen333 {
namespace thread {

/**
 * @brief Inverts lock/unlock operations on a lock
 * @tparam BasicLock lock type that supports lock() and unlock()
 */
template<typename BasicLock>
class lock_inverter {
  BasicLock &lock_;
 public:
  /**
   * @brief Creates the inverter, does not lock or unlock
   * @param lock lock to reverse
   */
  lock_inverter(BasicLock& lock) : lock_(lock) {}

 private:
  lock_inverter(const lock_inverter &) DELETE_METHOD;
  lock_inverter(lock_inverter &&) DELETE_METHOD;
  lock_inverter &operator=(const lock_inverter &) DELETE_METHOD;
  lock_inverter &operator=(lock_inverter &&) DELETE_METHOD;

 public:
  /**
   * @brief Unlocks the underlying lock
   */
  void lock() {
    lock_.unlock();
  }

  /**
   * @brief Locks the underlying lock
   */
  void unlock() {
    lock_.lock();
  }
};

/**
 * @brief Event primitive, acting like a turnstile
 *
 * A synchronization primitive that allows multiple threads to wait until the
 * event is notified.  The notifier can either `notify_one()` to let a single waiting
 * thread through (if any), or `notify_all()` to let all currently waiting threads through.
 *
 * Implementation is based on boost's boost/interpress/sync/detail/condition_algorithm_8a.hpp
 * Their implementation guarantees not to have spurious wake-ups
 *
 */
class event {

 public:

  /**
 * @brief Creates the event
 */
  event() : waiters_(), block_lock_(1), block_queue_(0), unblock_lock_(), external_() {}

  // disable copy/move constructors
 private:
  event(const event&);
  event(event&&);
  event& operator=(const event&);
  event& operator=(event&&);

 public:

  /**
   * @brief Waits for the event to be triggered
   *
   * Causes the current thread to block until either `notify_all()` is called, or `notify_one()` and this thread
   * happens to be the one awoken.  Note that order of wakes is system-dependent, and not necessarily in order of
   * arrival.  This event will <em>not</em> exhibit spurious wake-ups.  A thread will be forced
   * to wait here indefinitely until the event is triggered.
   */
  void wait() {
    std::unique_lock<std::mutex> lock(external_);
    wait(lock, false, std::chrono::steady_clock::now());
  }

  /**
   * @brief Waits for the event to be triggered or for a timeout period to elapse
   *
   * Causes the current thread to block until `notify_all()` is called, or `notify_one()` and this thread
   * happens to be the one awoken, or until the specified timeout period elapses, whichever comes first.
   * @tparam Rep timeout duration representation
   * @tparam Period timeout clock period
   * @param rel_time maximum relative time to wait for condition to be set
   * @return `true` if event is triggered, `false` if timeout has elapsed without event being triggered
   */
  template<class Rep, class Period>
  bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) {
    std::unique_lock<std::mutex> lock(external_);
    return wait(lock, true, std::chrono::steady_clock::now()+rel_time);
  }

  /**
    * @brief Waits for the event to be triggered or for a time-point to be reached
    *
    * Causes the current thread to block until `notify_all()` is called, or `notify_one()` and this thread
    * happens to be the one awoken, or until the specified timeout time has been reached, whichever comes first.
    *
    * @tparam Clock clock type
    * @tparam Duration clock duration type
    * @param timeout_time absolute timeout time
    * @return `true` if event is triggered, `false` if timeout time has been reached without event
    */
  template<class Clock, class Duration >
  bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time ) {
    std::unique_lock<std::mutex> lock(external_);
    return wait(lock, true, timeout_time);
  }

  /**
   * @brief Wake a single thread waiting for the event to be triggered
   *
   * Note that the choice of thread to be awoken is up to the underlying system.  Threads are not necessarily
   * notified in order of arrival.
   */
  void notify_one() {
    notify(false);
  }

  /**
   * @brief Wake all threads waiting for the event to be triggered
   *
   * All threads waiting for the event will be awoken and will continue.
   */
  void notify_all() {
    notify(true);
  }

 private:

  void notify(bool broadcast) {
    int signals;
    {
      // lock preventing new waiters from being signalled
      std::lock_guard<decltype(unblock_lock_)>  unblocklock(unblock_lock_);

      if ( waiters_.unblock != 0 ) {
        if ( waiters_.blocked == 0) {
          // nothing to signal
          return;
        }
        if (broadcast) {
          // move blocked to signals/unblock
          signals = waiters_.blocked;
          waiters_.unblock += signals;
          waiters_.blocked = 0;
        } else {
          // only signal one
          signals = 1;
          waiters_.unblock++;
          waiters_.blocked--;
        }
      } else if ( waiters_.blocked > waiters_.gone ) {
        block_lock_.wait();                      // close the gate
        if ( waiters_.gone != 0) {
          // waiters gone are no longer blocked
          waiters_.blocked -= waiters_.gone;
          waiters_.gone = 0;
        }
        if (broadcast) {
          // move blocked to unblock/signals
          signals = waiters_.blocked;
          waiters_.unblock = signals;
          waiters_.blocked = 0;
        } else {
          signals = 1;
          waiters_.unblock = 1;
          waiters_.blocked--;
        }
      } else {
        return;
      }
    }
    // post a number of signals
    while (signals > 0) {
      --signals;
      block_queue_.notify();
    }
  }

  template<class Clock, class Duration>
  bool wait(std::unique_lock<std::mutex>& lock,
            bool timeout, const std::chrono::time_point<Clock, Duration>& abs_time) {

    long signals_left = 0;
    long waiters_gone = 0;

    // scoped lock on memory to atomically change waiters count
    {
      cpen333::thread::semaphore_guard<decltype(block_lock_)> blocklock(block_lock_);
      ++(waiters_.blocked);
    }

    // release mutex and wait on semaphore until notify_all or notify_one called
    // unlock lock, and protect so will be relocked out of scope
    lock_inverter<std::unique_lock<std::mutex>> ilock(lock);
    std::lock_guard<lock_inverter<std::unique_lock<std::mutex>>> external_unlock(ilock);

    // wait on queue and see if timed out
    bool timed_out = false;
    if (timeout) {
      timed_out = !block_queue_.wait_until(abs_time);
    } else {
      block_queue_.wait();
    }

    // reduce waiters count and check if we were the last in a notify_all
    {
      std::lock_guard<std::mutex> unblocklock(unblock_lock_);
      signals_left = waiters_.unblock;

      if (signals_left != 0) {
        if (timed_out) {
          if (waiters_.blocked != 0) {
            --(waiters_.blocked);
          } else {
            ++(waiters_.gone);    // spurious wakeup
          }
        }

        if (--(waiters_.unblock) == 0) {
          if (waiters_.blocked != 0) {
            block_lock_.notify();   // allow one more through
            signals_left = 0;       // let him take care of rest
          } else {
            // take waiters_.gone out of shared memory
            waiters_gone = waiters_.gone;
            if (waiters_gone != 0) {
              waiters_.gone = 0;
            }
          } // blocked
        } // last to unblock
      } // signals left

        // timeout/cancelled or spurious semaphore
      else if ((std::numeric_limits<long>::max)() / 2 == ++(waiters_.gone)) {
        // reduce both blocked and gone
        cpen333::thread::semaphore_guard<decltype(block_lock_)> blocklock(block_lock_);
        waiters_.blocked -= waiters_.gone;
        waiters_.gone = 0;
      }
    } // unblock lock is unlocked

    // one signal left
    if (signals_left == 1) {
      if (waiters_gone != 0) {
        while (waiters_gone > 0) {
          --waiters_gone;
          block_queue_.wait();
        }
      }
      block_lock_.notify();  // allow next through
    }

    return !timed_out;
  }

  struct waiting_data {
    long blocked;   // number of waiters blocked
    long unblock;   // number of waiters to unblock
    long gone;      // number of waiters gone
    waiting_data() : blocked(0), unblock(0), gone(0) {}
  };

  waiting_data waiters_;
  cpen333::thread::semaphore block_lock_;
  cpen333::thread::semaphore block_queue_;
  std::mutex unblock_lock_;
  std::mutex external_;

};

} // process
} // cpen333

#endif //CPEN333_PROCESS_EVENT_H
