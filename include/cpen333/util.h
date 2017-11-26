/**
 * @file
 * @brief Utility functions
 *
 * Contains functions for formatting strings, testing for data on standard input, and printing
 * system library errors.
 */

#ifndef CPEN333_UTIL_H
#define CPEN333_UTIL_H

#include "os.h"
#if defined(WINDOWS) && !defined(__CYGWIN__)
// prevent windows max macro
#undef NOMINMAX
#define NOMINMAX 1
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <conio.h>
#else
// for _khbit()
#include <poll.h>
#include <unistd.h>
// #include <cstdio>
#endif
#include <cstdio>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

// if using c++11, explicitly delete constructors and assignment operators
// otherwise, just declare but not define them
#if __cplusplus >= 201102L
/**
 * @brief Delete constructor/assignment operator
 */
#define DELETE_METHOD = delete
#else
/**
* @brief Mark constructor/assignment as deleted
*/
#define DELETE_METHOD
#endif

/**
 * @brief Macro for avoiding unused variable warnings
 */
#define UNUSED(X) (void)(X)

namespace cpen333 {

#if !defined(WINDOWS)
namespace detail {

/**
 * @brief Keyboard hit function for POSIX
 * @return 0 if no keyboard input, >0 if data on stdin exists
 */
inline int _kbhit() {
  pollfd fds;
  fds.fd = STDIN_FILENO;
  fds.events = POLLIN | POLLHUP | POLLERR;
  fds.revents = 0;

  int status = ::poll(&fds, 1, 0);
  if (status > 0 && (fds.revents & 0x01) > 0) {
    return 1;
  }
  return 0;
}
} // detail
#endif

/**
 * @brief Tests for keyboard input
 *
 * Wrapper around platform-specific code for detecting the presence of data available on the
 * standard input stream `stdin`.  This call returns immediately, without blocking.
 *
 * @return non-zero value if there is content
 */
inline int test_stdin() {
#if defined(WINDOWS)
  return _kbhit();
#else
  return detail::_kbhit();
#endif
}

/**
 * @brief Print error message
 *
 * Attempts to detect the last system error that occured, interprets the corresponding error message,
 * and prints it to sterr (the standard error output stream, usually the console), preceding it with the
 * custom message specified in msg.
 *
 * On Windows, the error corresponds to that obtained by GetLastError().  On Linux and OSX, it is the
 * build-in `errno`.  The error message produced by perror is platform-depend.
 *
 * perror should be called right after the error was produced, otherwise it may be overwritten by calls
 * to other system functions.
 *
 * @param msg custom message
 */
inline void perror(const std::string& msg) {
#ifdef WINDOWS
  UINT LastError = GetLastError();
  char buff[512];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, LastError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buff, 1024, NULL);
  std::cerr << msg << ": " << buff << std::endl;
#else
  ::perror(msg.c_str());
#endif
}

/**
 * @brief Print message to standard error
 * @param msg custom message
 */
inline void error(const std::string& msg) {
  std::cerr << msg << std::endl;
}

/**
 * @brief Pause for input
 *
 * Mirrors the Windows system("pause") command in a cross-platform way, waiting for
 * keyboard input.
 */
inline void pause() {
  std::cin.clear();  // flush input
  std::cout << "Press ENTER to continue . . .";
  std::string line;
  std::getline(std::cin, line);
}

}// cpen333

#endif //CPEN333_UTIL_H
