/**
 * @file
 * @brief Windows implementation of an inter-process named mutex
 *
 * Uses a Windows mutex.
 */
#ifndef CPEN333_PROCESS_MUTEX_WINDOWS_H
#define CPEN333_PROCESS_MUTEX_WINDOWS_H

/**
 * @brief Suffix to append to mutex names for uniqueness
 */
#define MUTEX_NAME_SUFFIX "_mux"

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

namespace cpen333 {
namespace process {
namespace windows {

/**
 * @brief Inter-process named mutual exclusion primitive
 *
 * Used to limit resource access to one thread at a time
 *
 * This mutex has USAGE PERSISTENCE, meaning the mutex will continue to exist as long as at least one process/thread
 * is holding a reference to it.
 */
class mutex : public impl::named_resource_base {
 public:
  /**
   * @brief Alias to native handle type, which on Windows is type HANDLE
   */
  typedef HANDLE native_handle_type;

  /**
   * @copydoc cpen333::process::posix::mutex::mutex()
   */
  mutex(const std::string& name) :
    impl::named_resource_base(name + std::string(MUTEX_NAME_SUFFIX)) {
    handle_  = CreateMutexA(NULL, false, id_ptr()) ;
    if (handle_ == INVALID_HANDLE_VALUE) {
      cpen333::perror(std::string("Cannot create mutex ")+this->name());
    }
  }

  /**
   * @brief Destructor, closes this instance's handle to the shared mutex
   */
  ~mutex() {
    if (!CloseHandle(handle_)) {
      cpen333::perror(std::string("Cannot destroy mutex ")+name());
    }
  }

  /**
   * @copydoc cpen333::process::posix::mutex::lock()
   */
  void lock() {
    UINT result;
    // wait on handle_ until we are successful (ignore spurious wakes)
    do {
      result = WaitForSingleObject(handle_, INFINITE) ;
    } while (result != WAIT_OBJECT_0 && result != WAIT_FAILED);
    if (result == WAIT_FAILED) {
      cpen333::perror(std::string("Failed to lock mutex "+name()));
    }
  }

  /**
   * @copydoc cpen333::process::posix::mutex::try_lock()
   */
  bool try_lock() {
    // try locking with a 0 timeout
    UINT result = WaitForSingleObject(handle_, 0) ;
    if (result == WAIT_FAILED) {
      cpen333::perror(std::string("Failed to lock mutex "+name()));
    }

    // if we were actually signaled, return true
    return (result == WAIT_OBJECT_0);
  }

  /**
   * @copydoc cpen333::process::posix::mutex::try_lock_for()
   */
  template< class Rep, class Period >
  bool try_lock_for( const std::chrono::duration<Rep,Period>& timeout_duration ) {
    DWORD time = (DWORD)(std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration).count());
    UINT result = WaitForSingleObject(handle_, time) ;
    if (result == WAIT_FAILED) {
      cpen333::perror(std::string("Failed to lock mutex "+name()));
    }
    // if we were actually signaled, return true
    return (result == WAIT_OBJECT_0);
  }

  /**
   * @copydoc cpen333::process::posix::mutex::try_lock_until()
   */
  template< class Clock, class Duration >
  bool try_lock_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) {
    auto duration = timeout_time - std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    if (ms.count() < 0) {
      ms = std::chrono::milliseconds(0);
    }
    return try_lock_for(ms);
  }

  /**
   * @copydoc cpen333::process::posix::mutex::unlock()
   */
  bool unlock() {
    BOOL success = ReleaseMutex(handle_) ;  // FALSE on failure, TRUE on success
    if (!success) {
      cpen333::perror(std::string("Failed to unlock mutex "+name()));
    }
    return success != 0;
  }

  /**
   * @brief Returns a native handle
   *
   * In this case, a Windows HANDLE to the Mutex
   *
   * @return native handle to underlying mutex
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
 * @brief Alias to Windows implementation of a named mutex
 */
typedef windows::mutex mutex;


/**
 * @brief Alias to Windows implementation of a named mutex allowing timed waits
 */
typedef windows::mutex timed_mutex;

} // process
} // cpen333

// undef local macros
#undef MUTEX_NAME_SUFFIX

#endif //CPEN333_PROCESS_MUTEX_WINDOWS_H
