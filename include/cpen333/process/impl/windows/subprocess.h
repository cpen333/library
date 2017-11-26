/**
 * @file
 * @brief Windows implementation of a child process
 */
#ifndef CPEN333_PROCESS_WINDOWS_SUBPROCESS_H
#define CPEN333_PROCESS_WINDOWS_SUBPROCESS_H

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
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

/**
 * @brief Special code to indicate subprocess was manually terminated
 */
#define SUBPROCESS_TERMINATED 0xFFFFEEEE

namespace cpen333 {
namespace process {
namespace windows {

// WINDOWS
/**
 * @brief A child process
 *
 * Allows launching of a child process, with environment inherited from the current process.
 */
class subprocess {
 private:
  PROCESS_INFORMATION process_info_;
  std::string cmd_;
  bool detached_;
  bool started_;
  bool terminated_;

 public:
  /**
   * @brief Alias to native handle type, on Windows is a PROCESS_INFORMATION structure
   */
  typedef HANDLE native_handle_type;

  /**
   * @brief Constructs a new subprocess
   *
   * The new process will run the command exec[0] with argv parameters {exec[1], exec[2], ...}.  A detached child
   * process will run in a separate thread, (and on Windows, a separate window) concurrently with the parent process.
   * If not detached, the parent will wait for a running child to complete.
   *
   * @param exec command and arguments to execute
   * @param start whether to start the subprocess immediately
   * @param detached run the subprocess in `detached' mode
   */
  subprocess(const std::vector<std::string> &exec, bool start = true,
             bool detached = false) :
      process_info_(), cmd_(create_windows_command(exec)), detached_(detached),
      started_(false), terminated_(false) {
    if (start) {
      this->start();
    }
  }

  /**
  * @brief Constructs a new subprocess from a command string
  *
  * The new process will run the command cmd with argv parameters parsed from the string.  A detached child
  * process will run in a separate thread, (and on Windows, a separate window) concurrently with the parent process.
  * If not detached, the parent will wait for a running child to complete.
  *
  * @param cmd command and arguments to execute
  * @param start whether to start the subprocess immediately
  * @param detached run the subprocess in `detached' mode
  */
  subprocess(const std::string &cmd, bool start = true, bool detached = false) :
      process_info_(), cmd_(cmd), detached_(detached),
      started_(false), terminated_(false) {
    if (start) {
      this->start();
    }
  }

  /**
   * @copydoc cpen333::process::posix::subprocess::start()
   */
  bool start() {

    // already started
    if (started_) {
      return false;
    }

    int flags = 0; // process flags, create detached process
    if (detached_) {
      flags |= CREATE_NEW_CONSOLE; //DETACHED_PROCESS;
    }

    STARTUPINFOA	startup_info = {
        sizeof(STARTUPINFOA) ,
        NULL ,			// reserved
        NULL ,			// ignored in console applications
        (char *)(cmd_.c_str()) ,	// displayed in title bar for console applications
        0,0,			// dwx, dwy, offset of top left of new window relative to top left of screen in pixel
        // flags below must specify STARTF_USEPOSITION. Ignored for console apps'
        0,0,			// dwxsize, dwysize: Width and height of the window if new window specified
        // must use flags STARTF_USESIZE. Ignored for console apps'
        0,0,			// size of console in characters, only if STARTF_USECOUNTCHARS flag specified,
        // Ignored for console apps
        0,				// Colour control, for background and text. Ignored for console apps
        0,				// Flags. Ignored for console applications
        0,				// ignored unless showwindow flag set
        0 ,
        NULL,
        0,0,0			// stdin, stdout and stderr handles (inherited from parent)
    };

    // try to create a process
    BOOL success = CreateProcessA( NULL,	// application name
                                  (char*)cmd_.c_str(),
                                  NULL,			// process attributes
                                  NULL,			// thread attributes
                                  TRUE,			// inherits handles of parent
                                  flags,		// Priority and Window control flags,
                                  NULL,			// use environent of parent
                                  NULL,			// use same drive and directory as parent
                                  &startup_info,// controls appearance of process (see above)
                                  &process_info_	// Stored process handle and ID into this object
    );

    // check for error and print message if appropriate
    if (!success) {
      process_info_.dwProcessId = (DWORD)(-1); // signal bad process
      cpen333::perror(std::string("Failed to create process ")+cmd_);
    } else {
      started_ = true;
    }
    return success != 0;
  }

  /**
   * @copydoc cpen333::process::posix::subprocess::join()
   */
  bool join() {
    return wait_for_internal(INFINITE);
  }

  /**
   * @copydoc cpen333::process::posix::subprocess::wait()
   */
  bool wait() {
    if (terminated_) {
      return true;
    }
    return join();
  }

  /**
   * @copydoc cpen333::process::posix::subprocess::wait_for()
   */
  template<typename Rep, typename Period>
  bool wait_for(const std::chrono::duration<Rep,Period>& duration) {
    // convert to milliseconds
    DWORD time = (DWORD)(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    return wait_for_internal(time);
  }

  /**
   * @copydoc cpen333::process::posix::subprocess::wait_until()
   */
  template< class Clock, class Duration >
  bool wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) {
    auto now = std::chrono::steady_clock::now();
    DWORD time = 0;
    if (timeout_time > now) {
      // convert to milliseconds
      DWORD time = std::chrono::duration_cast<std::chrono::milliseconds>(timeout_time-now).count();
    }
    return wait_for_internal(time);
  }

  /**
   * @copydoc cpen333::process::posix::subprocess::terminated()
   */
  bool terminated() {
    if (terminated_) {
      return true;
    }
    // check wait
    return wait_for(std::chrono::milliseconds(0));
  }

  /**
   * @brief Force the process to terminate
   * @return true if successful
   */
  bool terminate() {
    auto success = TerminateProcess(process_info_.hProcess, SUBPROCESS_TERMINATED);
    return success != 0;
  }

 private:

  bool wait_for_internal(DWORD time) {
    if (terminated_) {
      return true;
    }

    UINT result = 0;
    if (process_info_.dwProcessId > 0) {
      result = WaitForSingleObject(get_process_handle(), time);
      if (result == WAIT_FAILED) {
        cpen333::perror(std::string("Failed to wait for process ") + cmd_);
      }
    }

    if (result == WAIT_OBJECT_0) {
      terminated_ = true;

      // close process and thread handles
      CloseHandle( process_info_.hProcess );
      CloseHandle( process_info_.hThread );
    }

    return terminated_;
  }

  // adapted from https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/
  //                      everyone-quotes-command-line-arguments-the-wrong-way/
  static void append_argument(const std::string &arg, std::string &commandline) {
    if (!arg.empty() && arg.find_first_of(" \t\n\v\"") == arg.npos) {
      commandline.append(arg);
    } else {
      // quote
      commandline.push_back('"');
      for (auto it = arg.begin(); it != arg.end(); ++it) {
        unsigned num_slashes = 0;

        while (it != arg.end() && *it == '\\') {
          ++it;
          ++num_slashes;
        }

        if (it == arg.end()) {
          //
          // Escape all backslashes, but let the terminating
          // double quotation mark we add below be interpreted
          // as a metacharacter.
          //
          commandline.append(num_slashes * 2, '\\');
          break;
        } else if (*it == '"') {
          //
          // Escape all backslashes and the following
          // double quotation mark.
          //
          commandline.append(num_slashes * 2 + 1, '\\');
          commandline.push_back(*it);
        } else {
          //
          // Backslashes aren't special here.
          //
          commandline.append(num_slashes, '\\');
          commandline.push_back(*it);
        }
      }
      commandline.push_back('"');
    }
  }

  // concatenate arguments into a proper command-line call
  static std::string create_windows_command(const std::vector<std::string>& exec) {
    std::string commandline; // build this
    append_argument(exec[0], commandline);
    for (size_t i = 1; i < exec.size(); ++i) {
      commandline.append(" ");
      append_argument(exec[i], commandline);
    }
    return commandline;
  }

  HANDLE get_process_handle() const { return process_info_.hProcess ; }		// get the process's handle
  HANDLE get_thread_handle() const { return process_info_.hThread ; }		// get the process's main thread handle
  UINT	 get_process_id() const { return process_info_.dwProcessId ; }		// get the process's ID
  UINT	 get_thread_id() const { return process_info_.dwThreadId; }		    // get the main thread's ID

};

} // native implementation

/**
 * @brief Alias to Windows implementation of a child process
 */
typedef windows::subprocess subprocess;

} // process
} // cpen333

// undef local macros
#undef SUBPROCESS_TERMINATED

#endif //CPEN333_PROCESS_WINDOWS_SUBPROCESS_H
