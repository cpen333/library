/**
 * @file
 * @brief First-in-first-out shared buffer
 */
#ifndef CPEN333_THREAD_FIFO_H
#define CPEN333_THREAD_FIFO_H

#include <string>
#include <chrono>
#include "../util.h"

#include <mutex>
#include "semaphore.h"

namespace cpen333 {
namespace thread {

/**
 * @brief Simple thread-safe first-in-first-out queue using a circular buffer.
 *
 * The buffer can only contain a single type of object.  Push will block until space is available
 * in the queue.  Pop will block until there is an item in the queue.
 *
 * @tparam ValueType type of data to store in the queue
 */
template<typename ValueType = unsigned long>
class fifo {

 public:

  /**
   * @brief data type stored in buffer
   */
  using value_type = ValueType;

  /**
   * @brief Creates a fifo
   * @param size the maximum number of elements that can be stored in the queue without blocking
   */
  fifo(size_t size = 1024) :
      info_{0, 0, size}, data_{nullptr}, // will initialize later
      pmutex_{}, cmutex_{},
      psem_{size},  // start at size of fifo
      csem_{0} {    // start at zero
    data_ = new ValueType[size];
  }

 private:
  fifo(const fifo &) DELETE_METHOD;
  fifo(fifo &&) DELETE_METHOD;
  fifo &operator=(const fifo &) DELETE_METHOD;
  fifo &operator=(fifo &&) DELETE_METHOD;

 public:

  /**
   * @brief Destructor
   *
   * Invalidates and frees any data in the queue
   */
  ~fifo() {
    // free data
    delete [] data;
  }

  /**
   * @brief Add a item to the fifo
   * @param val value to add
   */
  void push(const ValueType &val) {
    psem_.wait();   // wait until room to push
    push_item(val);
    csem_.notify(); // let consumer know a item is available
  }

  /**
   * @brief Add a item to the fifo
   * @param val value to add
   */
  void push(ValueType &&val) {
    psem_.wait();   // wait until room to push
    push_item(std::move(val));
    csem_.notify(); // let consumer know a item is available
  }

  /**
   * @brief Tries to add an item to the fifo without blocking
   *
   * If it is not possible to add to the fifo without blocking, then will return immediately without
   * adding the item.
   *
   * @param val item to add
   * @return `true` if item is added, `false` if would cause the current thread to block
   */
  bool try_push(const ValueType &val) {
    // see if room to push
    if (!psem_.try_wait()) {
      return false;
    }
    push_item(val);
    csem_.notify();  // let consumer know a item is available
    return true;
  }

  /**
   * @brief Tries to add an item to the fifo, will wait for a maximum amount of time before aborting
   *
   * If it is not possible to add the item to the fifo immediately, then the current thread
   * will block until either the item is added successfully, or a timeout period has elapsed.
   *
   * @tparam Rep duration representation
   * @tparam Period duration period
   * @param val value to add to the fifo
   * @param rel_time relative timeout time
   * @return `true` if item added within the timeout time, `false` if not added
   */
  template <typename Rep, typename Period>
  bool try_push_for(const ValueType& val, std::chrono::duration<Rep, Period>& rel_time) {
    return try_push_until(val, std::chrono::steady_clock::now()+rel_time);
  };

  /**
   * @brief Tries to add an item to the fifo, will wait until a timeout time is reached before aborting
   *
   * If it is not possible to add the item to the fifo immediately, then the current thread will block until
   * either the item is added successfully, or an absolute timeout time has been reached.
   *
   * @tparam Clock clock type
   * @tparam Duration clock duration type
   * @param val value to add to the fifo
   * @param timeout absolute timeout time
   * @return `true` if item added before the timeout time, `false` if not added
   */
  template<typename Clock, typename Duration>
  bool try_push_until(const ValueType& val, const std::chrono::time_point<Clock,Duration>& timeout) {
    // wait until room to push
    if (!psem_.wait_until(timeout)) {
      return false;
    }
    push_item(val);
    csem_.notify();  // let consumer know a item is available
    return true;
  };

  /**
   * @brief Removes the next item in the fifo
   *
   * Populates memory pointed to by `out` with next item in the fifo.  If there are no items in the fifo, then this
   * will block until one is available.
   *
   * @param out destination.  If `nullptr`, item is removed but not returned.
   */
  void pop(ValueType* out) {
    csem_.wait();      // wait until item available
    pop_item(out);
    psem_.notify();    // let producer know that we are done with the slot
  }

  /**
   * @brief Removes and returns the next item in the fifo
   *
   * If there are no items in the fifo, then this will block until one is available.
   *
   * @return next item in the fifo
   */
  ValueType pop() {
    ValueType out;
    pop(&out);
    return out;
  }

  /**
   * @brief Tries to remove and return the next item in the fifo without blocking
   *
   * Populates memory pointed to by `out` with the next item in the fifo.  If there are no items, then this
   * will return immediately without popping the item.
   *
   * @param out destination.  If `nullptr`, the item is removed but not returned.
   * @return `true` if item was successfully popped, `false` otherwise
   */
  bool try_pop(ValueType* out) {
    // see if data to pop
    if (!csem_.try_wait()) {
      return false;
    } 
    pop_item(out);
    psem_.notify();    // let producer know that we are done with the slot
    return true;
  }

  /**
   * @brief Tries to remove an item from the fifo, will wait for a maximum amount of time before aborting
   *
   * If it is not possible to remove an item from the fifo immediately, then the current thread
   * will block until either an item becomes available, or a timeout period has elapsed.
   *
   * @tparam Rep duration representation
   * @tparam Period duration period
   * @param out destination.  If `nullptr`, the item is removed but not returned
   * @param rel_time relative timeout time
   * @return `true` if item successfully popped, `false` if timeout elapsed
   */
  template <typename Rep, typename Period>
  bool try_pop_for(ValueType* out, std::chrono::duration<Rep, Period>& rel_time) {
    return try_pop_until(out, std::chrono::steady_clock::now()+rel_time);
  };

  /**
   * @brief Tries to remove an item from the fifo, will wait for a maximum timeout time to be reached before aborting
   *
   * If it is not possible to remove an item from the fifo immediately, then the current thread will block until either
   * an item becomes available, or a timeout time has been reached.
   *
   * @tparam Clock clock type
   * @tparam Duration clock duration type
   * @param out destination.  If `nullptr`, the item is removed but not returned
   * @param timeout absolute timeout time
   * @return `true` if item successfully popped, `false` if timeout
   */
  template<typename Clock, typename Duration>
  bool try_pop_until(ValueType* out, const std::chrono::time_point<Clock,Duration>& timeout) {
    // wait until room to push
    if (!csem_.wait_until(timeout)) {
      return false;
    }
    pop_item(out);
    psem_.notify();  // let consumer know a item is available
    return true;
  };

  /**
   * @brief Peeks at the next item in the fifo without removing it.
   *
   * Populates memory pointed to by `out` with next item in fifo.  If there are currently no items,
   * this method will block until an
   * item becomes available.  The item will remain in the fifo for the next `peek` or `pop` operation.
   *
   * @param out destination.  If `nullptr`, nothing happens.
   */
  void peek(ValueType* out) {
    csem_.wait();      // wait until item available
    peek_item(out);
  }

  /**
   * @brief Peeks at the next item in the fifo without removing it.
   *
   * Returns the next item in the fifo.  If there are currently no items, this method will block until one becomes
   * available.  The item will remain in the fifo for the next `peek` or `pop` operation.
   *
   * @return next item in the fifo.
   */
  ValueType peek() {
    ValueType out;
    peek(&out);
    return out;
  }

  /**
   * @brief Tries to peek at the next item in the fifo without blocking
   *
   * Populates memory pointed to by `out` with the next item in the fifo.  If there are no items, then this
   * will return immediately without peeking at the item. The item will remain in the fifo for the next `peek` or
   * `pop` operation.
   *
   * @param out destination.  If `nullptr`, nothing happens.
   * @return `true` if item was successfully peeked, `false` otherwise
   */
  bool try_peek(ValueType* out) {
    // see if data to peek
    if (!csem_.try_wait()) {
      return false;
    }
    peek_item(out);
    return true;
  }

  /**
   * @brief Tries to peek at the next item in the fifo, will wait for a maximum amount of time before aborting
   *
   * If it is not possible to peek at an item in the fifo immediately, then the current thread
   * will block until either an item becomes available, or a timeout period has elapsed.
   * The item will remain in the fifo for the next `peek` or `pop` operation.
   *
   * @tparam Rep duration representation
   * @tparam Period duration period
   * @param out destination.  If `nullptr`, nothing happens
   * @param rel_time relative timeout time
   * @return `true` if item successfully peeked, `false` if timeout elapsed
   */
  template <typename Rep, typename Period>
  bool try_peek_for(ValueType* out, std::chrono::duration<Rep, Period>& rel_time) {
    return try_peek_until(out, std::chrono::steady_clock::now()+rel_time);
  };

  /**
   * @brief Tries to peek at the next item in the fifo, will wait for a maximum timeout time before aborting
   *
   * If it is not possible to peek at the next item in the fifo immediately, then the current thread will block
   * until either an item becomes available, or a timeout time has been reached.
   * The item will remain in the fifo for the next `peek` or `pop` operation.
   *
   * @tparam Clock clock type
   * @tparam Duration clock duration type
   * @param out destination.  If `nullptr`, nothing happens
   * @param timeout absolute timeout time
   * @return `true` if item successfully peeked, `false` if timeout
   */
  template<typename Clock, typename Duration>
  bool try_peek_until(ValueType* out, const std::chrono::time_point<Clock,Duration>& timeout) {
    // wait until room to push
    if (!csem_.wait_until(timeout)) {
      return false;
    }
    peek_item(out);
    return true;
  };

  /**
   * @brief Number of items currently in the fifo
   *
   * This method should be used sparingly, since items could be added/removed during or immediately after the call,
   * making the result potentially unreliable.
   *
   * @return number of items
   */
  size_t size() {
    std::lock_guard<std::mutex> lock1(pmutex_);
    std::lock_guard<std::mutex> lock2(cmutex_);

    if (info_.pidx < info_.cidx) {
      return info_.size - info_.cidx+info_.pidx;
    }
    return info_.pidx-info_.cidx;
  }

  /**
   * @brief Check if fifo queue is currently empty
   *
   * This method should be used sparingly, since items could be added/removed during or immediately after the call,
   * making the result potentially unreliable
   *
   * @return `true` if empty, `false` otherwise
   */
  bool empty() {
    std::lock_guard<std::mutex> lock1(pmutex_);
    std::lock_guard<std::mutex> lock2(cmutex_);
    return info_->pidx == info_->cidx;
  }

 private:

  // only to be called internally, does not wait for semaphore
  void push_item(const ValueType &val) {

    int loc = 0;
    {
      // look at index, protect memory from multiple simultaneous pushes
      std::lock_guard<std::mutex> lock(pmutex_);
      loc = info_.pidx;
      // increment producer index for next item, wrap around if at end
      if ((++info_.pidx) == info_.size) {
        info_.pidx = 0;
      }

      // copy data to correct location
      data_[loc] = val;  // add item to fifo

      // lock will unlock here as guard runs out of scope
    }

  }

  // only to be called internally, does not wait for semaphore
  void push_item(ValueType &&val) {
    int loc = 0;
    {
      // look at index, protect memory from multiple simultaneous pushes
      std::lock_guard<std::mutex> lock(pmutex_);
      loc = info_.pidx;
      // increment producer index for next item, wrap around if at end
      if ((++info_.pidx) == info_.size) {
        info_.pidx = 0;
      }

      // copy data to correct location
      data_[loc] = std::move(val);  // add item to fifo

      // lock will unlock here as guard runs out of scope
    }
  }

  void peek_item(ValueType* val) {
    int loc = 0;  // will store location of item to take
    {
      // look at index, protect memory from multiple simultaneous pops
      std::lock_guard<std::mutex> lock(cmutex_);
      loc = info_.cidx;
      // copy data to output
      if (val != nullptr) {
        *val = data_[loc];  // copy item out
      }
      // lock will unlock here as guard runs out of scope
    }
  }

  void pop_item(ValueType* val) {
    int loc = 0;  // will store location of item to take
    {
      // look at index, protect memory from multiple simultaneous pops
      std::lock_guard<std::mutex> lock(cmutex_);
      loc = info_.cidx;

      // increment consumer index for next item, wrap around if at end
      if ( (++info_.cidx) == info_.size) {
        info_.cidx = 0;
      }
      // copy data to output
      if (val != nullptr) {
        *val = std::move(data_[loc]);  // move item out
      }
      // lock will unlock here as guard runs out of scope
    }
  }

  struct fifo_info {
    size_t pidx;      // producer index
    size_t cidx;      // consumer index
    size_t size;      // size (in counts of ValueType)
    int initialized;  // magic initialized marker
  };

  fifo_info info_;                          // pointer to fifo information, will be at start of memory_
  ValueType* data_;                         // pointer to data in fifo, after info_ in memory
  std::mutex pmutex_;                       // mutex for protecting memory modified by producers
  std::mutex cmutex_;                       // mutex for protecting memory modified by consumers
  cpen333::thread::semaphore psem_;         // semaphore controlling when producer can add an item
  cpen333::thread::semaphore csem_;         //     "            "         consumer can remove an item

};

} // thread
} // cpen333

#endif //CPEN333_THREAD_FIFO_H
