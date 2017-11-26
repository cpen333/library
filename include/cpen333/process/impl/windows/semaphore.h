/**
 * @file
 * @brief Windows implementation of an inter-process named semaphore
 *
 * Uses a Windows Semaphore
 */
#ifndef CPEN333_PROCESS_WINDOWS_SEMAPHORE_H
#define CPEN333_PROCESS_WINDOWS_SEMAPHORE_H

#include <climits>
#include <string>
#include <chrono>
// prevent windows max macro
#undef NOMINMAX
/**
 * @brief Prevent windows from defining min(), max() macros
 */
#define NOMINMAX 1
#ifndef WIN32_LEAN_AND_MEAN
/**
* @brief Prevent inclusion of old winsock
*/
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "../../../util.h"
#include "../named_resource_base.h"

/**
 * @brief Maximum possible size of a semaphore
 */
#define MAX_SEMAPHORE_SIZE LONG_MAX

/**
 * @brief Suffix appended to semaphore names for uniqueness
 */
#define SEMAPHORE_NAME_SUFFIX "_sem"

namespace cpen333 {
namespace process {
namespace windows {

/**
 * @brief Inter-process named semaphore primitive
 *
 * Used to limit access to a number of resources.  Contains an integer whose value is never allowed to fall below zero.
 * There are two main supported actions: wait(), which decrements the internal value, and notify() which increments the
 * value.  If the value of the semaphore is zero, then wait() will cause the thread to block until the value becomes
 * greater than zero.
 *
 * This implementation has no explicit maximum value
 *
 * This semaphore has USAGE PERSISTENCE, meaning the mutex will continue to exist as long as at least one process/thread
 * is holding a reference to it.
 *
 */
class semaphore : public impl::named_resource_base {
 public:
  /**
   * @brief Alias to native handle type for semaphore
   *
   * In this case, a Windows HANDLE to a Semaphore
   */
  typedef HANDLE native_handle_type;

  /**
   * @copydoc cpen333::process::posix::semaphore::semaphore()
   */
  semaphore(const std::string& name, size_t value = 1) :
      impl::named_resource_base(name+std::string(SEMAPHORE_NAME_SUFFIX)), handle_(NULL) {

    // create named semaphore
    handle_ = CreateSemaphoreA(NULL, (LONG)value, (LONG)MAX_SEMAPHORE_SIZE, id_ptr());
    if (handle_ == INVALID_HANDLE_VALUE) {
       cpen333::perror(std::string("Cannot create semaphore ")+this->name());
    }
  }

  /**
   * @brief Destructor
   */
  ~semaphore() {
    // close the semaphore
    if (!CloseHandle(handle_)) {
      cpen333::perror(std::string("Cannot destroy semaphore ")+name());
    }
  }

  /**
   * @copydoc cpen333::process::posix::semaphore::value()
   */
  size_t value() {
    LONG val = 0;
    // check if one available, if so grab it, otherwise count was zero
    if( WAIT_OBJECT_0 == WaitForSingleObject(handle_,0L)) {
      // Semaphores count is at least one, determine previous value then release it
      if (!ReleaseSemaphore(handle_, 1, &val)) {
        cpen333::perror(std::string("Cannot get semaphore value ") + name());
      }
      ++val;  // reduce count by 1 to account for artificial wait
    }
    return val;
  }

  /**
   * @copydoc cpen333::process::posix::semaphore::wait()
   */
  void wait() {
    UINT result;
    // wait on handle_ until we are successful
    do {
      result = WaitForSingleObject(handle_, INFINITE) ;
    } while (result != WAIT_OBJECT_0 && result != WAIT_FAILED);
    // check if we had an error
    if (result == WAIT_FAILED) {
      cpen333::perror(std::string("Failed to wait on semaphore ")+name());
    }
  }

  /**
   * @copydoc cpen333::process::posix::semaphore::try_wait()
   */
  bool try_wait() {
    // try locking with a 0 timeout
    UINT result = WaitForSingleObject(handle_, 0) ;
    if (result == WAIT_FAILED) {
      cpen333::perror(std::string("Failed to wait on semaphore ")+name());
    }
    // if we were actually signaled, return true
    return (result == WAIT_OBJECT_0);
  }

  /**
   * @copydoc cpen333::process::posix::semaphore::wait_for()
   */
  template< class Rep, class Period >
  bool wait_for( const std::chrono::duration<Rep,Period>& timeout_duration ) {
    DWORD time = (DWORD)(std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration).count());
    if (time < 0) {
      time = 0;
    }
    UINT result = WaitForSingleObject(handle_, time) ;
    if (result == WAIT_FAILED) {
      cpen333::perror(std::string("Failed to wait for semaphore ")+name());
    }

    return (result == WAIT_OBJECT_0);
  }

  /**
   * @copydoc cpen333::process::posix::semaphore::wait_until()
   */
  template< class Clock, class Duration >
  bool wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) {
    auto duration = timeout_time - std::chrono::steady_clock::now();
    return wait_for(duration);
  }

  /**
   * @copydoc cpen333::process::posix::semaphore::notify()
   */
  void notify() {
    BOOL success = ReleaseSemaphore(handle_, 1, NULL) ;  // FALSE on failure, TRUE on success
    if (!success) {
      cpen333::perror(std::string("Failed to post semaphore ")+name());
    }
  }

  /**
   * @brief Returns a native handle to the semaphore
   *
   * The native handle has a type aliased to semaphore::native_handle_type
   *
   * On Windows systems, is of type HANDLE to a Semaphore.
   *
   * @return native semaphore handle
   */
  native_handle_type native_handle() {
    return handle_;
  }

  bool unlink() {
    return false;
  }

  /**
   * @copydoc cpen333::process::named_resource::unlink(const std::string&)
   */
  static bool unlink(const std::string& name) {
    UNUSED(name);
    return false;
  }

 private:
  native_handle_type handle_;

};

} // native implementation

/**
 * @brief Alias to a Windows implementation of a semaphore
 */
typedef windows::semaphore semaphore;

} // process
} // cpen333

// undef local macros
#undef MAX_SEMAPHORE_SIZE
#undef SEMAPHORE_NAME_SUFFIX

#endif //CPEN333_PROCESS_WINDOWS_SEMAPHORE_H
