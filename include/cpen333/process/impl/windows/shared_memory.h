/**
 * @file
 * @brief Windows implementation of an inter-process named shared memory (i.e. datapool)
 *
 * Uses a Windows memory-mapped file
 */
#ifndef CPEN333_PROCESS_WINDOWS_SHARED_MEMORY_H
#define CPEN333_PROCESS_WINDOWS_SHARED_MEMORY_H

/**
 * @brief Suffix to append to shared memory names for uniqueness
 */
#define SHARED_MEMORY_NAME_SUFFIX "_shm"

#include <string>
#include <cstdint>
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
 * @brief Inter-process shared memory implementation
 *
 * Creates and shares a block of memory between threads/processes, accessible using a unique name.  The block
 * of memory is mapped to DIFFERENT address spaces on each process.  This is essentially a
 * memory-mapped file.
 *
 * This shared memory has USAGE PERSISTENCE, meaning the mutex will continue to exist as long as at least one
 * process/thread is holding a reference to it.
 */
class shared_memory : public impl::named_resource_base {
 public:
  /**
   * @brief Alias to native handle for shared memory
   */
  typedef HANDLE native_handle_type;

  /**
   * @copydoc cpen333::process::posix::shared_memory::shared_memory()
   */
  shared_memory(const std::string &name, size_t size, bool readonly = false ) :
      impl::named_resource_base(name+std::string(SHARED_MEMORY_NAME_SUFFIX)),
      handle_(NULL),
      data_(nullptr),
      size_(size) {

    // Clear thread error, create mapping, then check if already exists
    SetLastError(0);
    handle_ = CreateFileMappingA(INVALID_HANDLE_VALUE,  // create in paging file
                                NULL,
                                PAGE_READWRITE,        // always read-write so we can initialize if we need to
                                0,
                                (DWORD)size,
                                id_ptr() );
    if (handle_ == INVALID_HANDLE_VALUE) {
      cpen333::perror(std::string("Cannot create shared memory ") + this->name());
      return;
    }
    // see if we need to initialize
    // bool initialize = GetLastError() != ERROR_ALREADY_EXISTS;

    // map (if not already mapped
    int flags = (readonly ? FILE_MAP_READ : FILE_MAP_WRITE);
    data_ = MapViewOfFile(
        handle_,            // file-mapping object to map into address space
        flags,              // read-write
        0, 0,               // offset
        size                // number of bytes to map, 0 means all
    );

    if (data_ == NULL) {
      cpen333::perror(std::string("Cannot map shared memory ") + this->name());
      return;
    }
  }

  /**
   * @copydoc cpen333::process::posix::shared_memory::~shared_memory()
   */
  ~shared_memory() {
    // unmap memory
    if (data_ != nullptr) {
      BOOL success = UnmapViewOfFile(data_);
      if (!success) {
        cpen333::perror(std::string("Cannot unmap shared memory ") + name());
      }
    }

    if (handle_ != NULL && !CloseHandle(handle_)) {
      cpen333::perror(std::string("Cannot close shared memory handle ") + name());
    }
  }

  /**
   * @copydoc cpen333::process::posix::shared_memory::get(size_t)
   */
  void* get(size_t offset = 0) {
    return (void*)((uint8_t*)data_ + offset);
  }

  /**
   * @copydoc cpen333::process::posix::shared_memory::operator[]()
   */
  uint8_t& operator[](size_t offset) {
    return *((uint8_t*)get(offset));
  }

  /**
   * @copydoc cpen333::process::posix::shared_memory::get(size_t)
   */
  template<typename T>
  T* get(size_t offset) {
    return (T*)get(offset);
  }

  /**
   * @copydoc cpen333::process::posix::shared_memory::get()
   */
  template<typename T>
  T* get() {
    return (T*)data_;
  }

  /**
   * @brief Native handle to underlying shared memory block
   *
   * On Windows systems, this is a Windows HANDLE to the memory-mapped file mapping
   *
   * @return native handle to shared memory block
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
  void* data_;       // pointer to shared data
  size_t size_;      // size of memory block

};

} // native implementation

/**
 * @brief Alias to Windows native implementation of inter=process shared memory
 */
typedef windows::shared_memory shared_memory;

} // process
} // cpen333

// undef local macros
#undef SHARED_MEMORY_NAME_SUFFIX

#endif //CPEN333_PROCESS_WINDOWS_SHARED_MEMORY_H
