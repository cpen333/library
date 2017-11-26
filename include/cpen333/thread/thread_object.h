/**
 * @file
 * @brief Base object-oriented thread object
 */
#ifndef CPEN333_THREAD_THREAD_OBJECT_H
#define CPEN333_THREAD_THREAD_OBJECT_H

#include <thread>
#include "../util.h"

namespace cpen333 {
namespace thread {

/**
 * @brief Base object-oriented thread object
 *
 * Virtual object with a main() method that must be overridden which will execute once the thread
 * is started.  Also has functionality to return the returned value of main().
 *
 * The thread is NOT started automatically (nor should it be due to memory access concerns).
 *
 */
class thread_object {
 private:
  std::thread* thread_;        // underlying thread object
  volatile bool terminated_;   // whether thread is terminated
  volatile int result_;

 public:
  /**
   * @brief Constructs the thread base
   */
  thread_object() : thread_(nullptr), terminated_(false), result_(0) {}

  /**
   * @brief Move constructor for thread for passing around
   * @param other object to move
   */
  thread_object(thread_object && other) :
      thread_(other.thread_), terminated_(other.terminated_), result_(other.result_) {
    other.thread_ = nullptr;
  }

  /**
   * @brief Move assignment operator for thread to move around
   * @param other thread to move
   * @return new thread
   */
  thread_object &operator=(thread_object && other) {
    thread_ = other.thread_;
    terminated_ = other.terminated_;
    result_ = other.result_;
    other.thread_ = nullptr;
    return *this;
  }

 private:
  thread_object(const thread_object &) DELETE_METHOD;
  thread_object &operator=(const thread_object &) DELETE_METHOD;

 public:
  
  /**
   * @brief Destructor, frees the thread
   */
  ~thread_object() {
    // free up thread
    if (thread_ != nullptr) {
      delete thread_;
      thread_ = nullptr;
    }
  }

  /**
   * @brief Start thread execution
   *
   * Runs the main() method of this thread object
   */
  void start() {
    if (thread_ == nullptr) {
      thread_ = new std::thread(&thread_object::__run, this);
    }
  }

  /**
   * @brief Waits for thread to finish executing.
   *
   * If thread hasn't been started yet, then it is started here.
   *
   * @return the result of main()
   */
  int join() {
    if (thread_ == nullptr) {
      start();
    }
    if (thread_->joinable()) {
      thread_->join();            // wait for parent thread to join, ensures result_ is set
    }
    return result_;
  }

  /**
   * @brief Allows the thread to execute independently from the thread's handle
   */
  void detach() {
    if (thread_ != nullptr) {
      thread_->detach();
    }
  }

  /**
   * @brief Checks whether the thread is joinable
   *
   * i.e. the thread has not already been joined, and is not detached
   *
   * @return true if thread is running and joinable, false otherwise
   */
  bool joinable() {
    if (thread_ != nullptr) {
      return thread_->joinable();
    }
    return false;
  }

  /**
   * @brief Checks whether thread object has finished executing
   * @return true if thread is terminated, false otherwise
   */
  bool terminated() {
    return terminated_;
  }

 protected:

  /**
   * @brief Main execution method of thread
   * @return main return value
   */
  virtual int main() = 0;

 private:
  // private non-virtual internal method that calls main
  void __run() {
    result_ = main();
    terminated_ = true;
  }
};

} // thread
} // cpen333

#endif // CPEN333_THREAD_THREAD_OBJECT_H