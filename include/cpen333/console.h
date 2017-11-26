/**
 * @file
 * @brief Utility class for manipulating the console
 */

#ifndef CPEN333_CONSOLE_H
#define CPEN333_CONSOLE_H

// #define NO_AIX  // use standard ANSI codes if AIX codes do not work

#include <iostream>

#include "os.h"
#include "util.h"

#ifdef WINDOWS
// prevent windows max macro
#undef NOMINMAX
#define NOMINMAX 1
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cstdio>   // for safe printf
#endif

namespace cpen333 {

/**
 * @brief Colours for foreground/background
 *
 * Colour definitions used for setting the foreground and background colours.
 */
enum color {
  BLACK, DARK_RED, DARK_GREEN, DARK_YELLOW, DARK_BLUE, DARK_MAGENTA, DARK_CYAN, LIGHT_GREY,
  DARK_GREY, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, DEFAULT
};

namespace detail {

/**
 * @brief ANSI codes used for POSIX terminal formatting and manipulation
 */
struct ANSI_CODES {
  static std::string FOREGROUND_COLOR_BLACK() {  return "\u001B[30m"; }
  static std::string FOREGROUND_COLOR_DARK_RED() {  return "\u001B[31m"; }
  static std::string FOREGROUND_COLOR_DARK_GREEN() {  return "\u001B[32m"; }
  static std::string FOREGROUND_COLOR_DARK_YELLOW() {  return "\u001B[33m"; }
  static std::string FOREGROUND_COLOR_DARK_BLUE() {  return "\u001B[34m"; }
  static std::string FOREGROUND_COLOR_DARK_MAGENTA() {  return "\u001B[35m"; }
  static std::string FOREGROUND_COLOR_DARK_CYAN() {  return "\u001B[36m"; }
  static std::string FOREGROUND_COLOR_LIGHT_GREY() {  return "\u001B[37m"; }
  static std::string FOREGROUND_COLOR_DEFAULT() {  return "\u001B[39m"; }

  static std::string BACKGROUND_COLOR_BLACK() {  return "\u001B[40m"; }
  static std::string BACKGROUND_COLOR_DARK_RED() {  return "\u001B[41m"; }
  static std::string BACKGROUND_COLOR_DARK_GREEN() {  return "\u001B[42m"; }
  static std::string BACKGROUND_COLOR_DARK_YELLOW() {  return "\u001B[43m"; }
  static std::string BACKGROUND_COLOR_DARK_BLUE() {  return "\u001B[44m"; }
  static std::string BACKGROUND_COLOR_DARK_MAGENTA() {  return "\u001B[45m"; }
  static std::string BACKGROUND_COLOR_DARK_CYAN() {  return "\u001B[46m"; }
  static std::string BACKGROUND_COLOR_LIGHT_GREY() {  return "\u001B[47m"; }
  static std::string BACKGROUND_COLOR_DEFAULT() {  return "\u001B[49m"; }

#ifdef NO_AIX
  // standard ANSI versions (uses "bold")
  static std::string FOREGROUND_COLOR_DARK_GREY() {  return "\u001B[30;1m"; }
  static std::string FOREGROUND_COLOR_RED() {  return "\u001B[31;1m"; }
  static std::string FOREGROUND_COLOR_GREEN() {  return "\u001B[32;1m"; }
  static std::string FOREGROUND_COLOR_YELLOW() {  return "\u001B[33;1m"; }
  static std::string FOREGROUND_COLOR_BLUE() {  return "\u001B[34;1m"; }
  static std::string FOREGROUND_COLOR_MAGENTA() {  return "\u001B[35;1m"; }
  static std::string FOREGROUND_COLOR_CYAN() {  return "\u001B[36;1m"; }
  static std::string FOREGROUND_COLOR_WHITE() {  return "\u001B[37;1m"; }

  static std::string BACKGROUND_COLOR_DARK_GREY() {  return "\u001B[40;1m"; }
  static std::string BACKGROUND_COLOR_RED() {  return "\u001B[41;1m"; }
  static std::string BACKGROUND_COLOR_GREEN() {  return "\u001B[42;1m"; }
  static std::string BACKGROUND_COLOR_YELLOW() {  return "\u001B[43;1m"; }
  static std::string BACKGROUND_COLOR_BLUE() {  return "\u001B[44;1m"; }
  static std::string BACKGROUND_COLOR_MAGENTA() {  return "\u001B[45;1m"; }
  static std::string BACKGROUND_COLOR_CYAN() {  return "\u001B[46;1m"; }
  static std::string BACKGROUND_COLOR_WHITE() {  return "\u001B[47;1m"; }
#else
  // AIX versions
  static std::string FOREGROUND_COLOR_DARK_GREY() {  return "\u001B[90m"; }
  static std::string FOREGROUND_COLOR_RED() {  return "\u001B[91m"; }
  static std::string FOREGROUND_COLOR_GREEN() {  return "\u001B[92m"; }
  static std::string FOREGROUND_COLOR_YELLOW() {  return "\u001B[93m"; }
  static std::string FOREGROUND_COLOR_BLUE() {  return "\u001B[94m"; }
  static std::string FOREGROUND_COLOR_MAGENTA() {  return "\u001B[95m"; }
  static std::string FOREGROUND_COLOR_CYAN() {  return "\u001B[96m"; }
  static std::string FOREGROUND_COLOR_WHITE() {  return "\u001B[97m"; }

  static std::string BACKGROUND_COLOR_DARK_GREY() {  return "\u001B[100m"; }
  static std::string BACKGROUND_COLOR_RED() {  return "\u001B[101m"; }
  static std::string BACKGROUND_COLOR_GREEN() {  return "\u001B[102m"; }
  static std::string BACKGROUND_COLOR_YELLOW() {  return "\u001B[103m"; }
  static std::string BACKGROUND_COLOR_BLUE() {  return "\u001B[104m"; }
  static std::string BACKGROUND_COLOR_MAGENTA() {  return "\u001B[105m"; }
  static std::string BACKGROUND_COLOR_CYAN() {  return "\u001B[106m"; }
  static std::string BACKGROUND_COLOR_WHITE() {  return "\u001B[107m"; }
#endif

  static std::string COLOR_RESET() {  return "\u001B[0m"; }
  static std::string COLOR_REVERSE() {  return "\u001B[7m"; }
  static std::string COLOR_UNREVERSE() {  return "\u001B[27m"; }

  static std::string ERASE_DISPLAY_TO_END() {  return "\u001B[0J"; }
  static std::string ERASE_DISPLAY_TO_BEGINNING() {  return "\u001B[1J"; }
  static std::string ERASE_DISPLAY() {  return "\u001B[2J"; }

  static std::string ERASE_LINE_TO_END() {  return "\u001B[0K"; }
  static std::string ERASE_LINE_TO_BEGINNING() {  return "\u001B[1K"; }
  static std::string ERASE_LINE() {  return "\u001B[2K"; }

  static std::string HIDE_CURSOR() {  return "\u001B[?25l"; }
  static std::string SHOW_CURSOR() {  return "\u001B[?25h"; }

  // NOTE: these are format strings taking positions
  static std::string CURSOR_HORIZONTAL_ABSOLUTE() {  return "\u001B[%iG"; }
  static std::string CURSOR_POSITION() {  return "\u001B[%i;%iH"; }

};

#ifdef WINDOWS
class console_handler {
  static const int FOREGROUND_MASK = 0x0F;
  static const int BACKGROUND_MASK = 0xF0;
  int default_attributes_;
 public:

  console_handler() {
    // get current color
    default_attributes_ = get_text_attributes();
  }
  
  void set_foreground_color(const color &color) {
    // clear foreground color
    int cflags = 0;
    switch (color) {
      case BLACK: {
        break;
      }
      case DARK_RED: {
        cflags |= FOREGROUND_RED;
        break;
      }
      case DARK_GREEN: {
        cflags |= FOREGROUND_GREEN;
        break;
      }
      case DARK_YELLOW: {
        cflags |= (FOREGROUND_RED | FOREGROUND_GREEN);
        break;
      }
      case DARK_BLUE: {
        cflags |= FOREGROUND_BLUE;
        break;
      }
      case DARK_MAGENTA: {
        cflags |= (FOREGROUND_RED | FOREGROUND_BLUE);
        break;
      }
      case DARK_CYAN: {
        cflags |= (FOREGROUND_GREEN | FOREGROUND_BLUE);
        break;
      }
      case LIGHT_GREY: {
        cflags |= (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        break;
      }
      case DARK_GREY: {
        cflags |= FOREGROUND_INTENSITY;
        break;
      }
      case RED: {
        cflags |= (FOREGROUND_RED | FOREGROUND_INTENSITY);
        break;
      }
      case GREEN: {
        cflags |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
      }
      case YELLOW: {
        cflags |= (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        break;
      }
      case BLUE: {
        cflags |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
      }
      case MAGENTA: {
        cflags |= (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        break;
      }
      case CYAN: {
        cflags |= (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        break;
      }
      case WHITE: {
        cflags |= (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        break;
      }
      case DEFAULT: {
        cflags = FOREGROUND_MASK & default_attributes_;
        break;
      }
    }

    // flush everything
    std::cout.flush();
    fflush(stdout);
    // get current color
    int current = (get_text_attributes() & ~FOREGROUND_MASK);
    set_text_attributes(cflags | current);
  }

  void set_background_color(const color &color) {
    // clear foreground color
    int cflags = 0;
    switch (color) {
      case BLACK: {
        break;
      }
      case DARK_RED: {
        cflags |= BACKGROUND_RED;
        break;
      }
      case DARK_GREEN: {
        cflags |= BACKGROUND_GREEN;
        break;
      }
      case DARK_YELLOW: {
        cflags |= (BACKGROUND_RED | BACKGROUND_GREEN);
        break;
      }
      case DARK_BLUE: {
        cflags |= BACKGROUND_BLUE;
        break;
      }
      case DARK_MAGENTA: {
        cflags |= (BACKGROUND_RED | BACKGROUND_BLUE);
        break;
      }
      case DARK_CYAN: {
        cflags |= (BACKGROUND_GREEN | BACKGROUND_BLUE);
        break;
      }
      case LIGHT_GREY: {
        cflags |= (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
        break;
      }
      case DARK_GREY: {
        cflags |= BACKGROUND_INTENSITY;
        break;
      }
      case RED: {
        cflags |= (BACKGROUND_RED | BACKGROUND_INTENSITY);
        break;
      }
      case GREEN: {
        cflags |= BACKGROUND_GREEN | BACKGROUND_INTENSITY;
        break;
      }
      case YELLOW: {
        cflags |= (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
        break;
      }
      case BLUE: {
        cflags |= BACKGROUND_BLUE | BACKGROUND_INTENSITY;
        break;
      }
      case MAGENTA: {
        cflags |= (BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
        break;
      }
      case CYAN: {
        cflags |= (BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
        break;
      }
      case WHITE: {
        cflags |= (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
        break;
      }
      case DEFAULT: {
        cflags = BACKGROUND_MASK & default_attributes_;
        break;
      }
    }

    // flush everything
    std::cout.flush();
    fflush(stdout);
    // get current color
    int current = (get_text_attributes() & ~BACKGROUND_MASK);
    set_text_attributes(cflags | current);
  }

  void reset_colors() {
    static const int COLOR_MASK = BACKGROUND_MASK | FOREGROUND_MASK | COMMON_LVB_REVERSE_VIDEO;
    int current = get_text_attributes() & ~COLOR_MASK;
    set_text_attributes(current | (default_attributes_ & COLOR_MASK));
  }

  void set_colors_reverse(bool set) {
    int current = get_text_attributes();
    if (set) {
      current |= COMMON_LVB_REVERSE_VIDEO;
    } else {
      current &= ~COMMON_LVB_REVERSE_VIDEO;
    }
    set_text_attributes(current); // flip reverse bit
  }

  void set_cursor_position(int r, int c) {
    COORD coord = {(short)c, (short)r};
    HANDLE hstdout = get_stdout_handle();
    SetConsoleCursorPosition(hstdout, coord);
  }

  void clear_display() {
    // Adapted from:  http://www.cplusplus.com/articles/4z18T05o/#Windows
    HANDLE hstdout = get_stdout_handle();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    // get screen info
    if (!GetConsoleScreenBufferInfo( hstdout, &csbi )) return;

    DWORD cellcount = csbi.dwSize.X *csbi.dwSize.Y;
    COORD home = { (short)0, (short)0 };
    DWORD count;

    // Fill the entire buffer with spaces and attributes
    if (!FillConsoleOutputCharacter( hstdout, (TCHAR)' ', cellcount, home, &count)) return;
    if (!FillConsoleOutputAttribute( hstdout, csbi.wAttributes, cellcount, home, &count)) return;
  }

  void clear_line() {
    HANDLE hstdout = get_stdout_handle();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    // get screen info
    if (!GetConsoleScreenBufferInfo( hstdout, &csbi )) return;

    DWORD cellcount = csbi.dwSize.X;
    COORD home = { (short)0, (short)csbi.dwCursorPosition.Y };
    DWORD count;

    // Fill the entire buffer with spaces and attributes
    if (!FillConsoleOutputCharacter( hstdout, (TCHAR)' ', cellcount, home, &count)) return;
    if (!FillConsoleOutputAttribute( hstdout, csbi.wAttributes, cellcount, home, &count)) return;
  }

  void clear_line_right() {
    HANDLE hstdout = get_stdout_handle();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    // get screen info
    if (!GetConsoleScreenBufferInfo( hstdout, &csbi )) return;

    DWORD cellcount = csbi.dwSize.X-csbi.dwCursorPosition.X;
    COORD home = { (short)csbi.dwCursorPosition.X, (short)csbi.dwCursorPosition.Y };
    DWORD count;

    // Fill the entire buffer with spaces and attributes
    if (!FillConsoleOutputCharacter( hstdout, (TCHAR)' ', cellcount, home, &count)) return;
    if (!FillConsoleOutputAttribute( hstdout, csbi.wAttributes, cellcount, home, &count)) return;
  }

  void clear_line_left() {
    HANDLE hstdout = get_stdout_handle();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    // get screen info
    if (!GetConsoleScreenBufferInfo( hstdout, &csbi )) return;

    DWORD cellcount = csbi.dwCursorPosition.X+1;
    COORD home = { (short)0, (short)csbi.dwCursorPosition.Y };
    DWORD count;

    // Fill the entire buffer with spaces and attributes
    if (!FillConsoleOutputCharacter( hstdout, (TCHAR)' ', cellcount, home, &count)) return;
    if (!FillConsoleOutputAttribute( hstdout, csbi.wAttributes, cellcount, home, &count)) return;
  }
  
  void set_cursor_visible(bool visible) {
    CONSOLE_CURSOR_INFO cci = {1, visible};
    SetConsoleCursorInfo(get_stdout_handle(), &cci);
  }

  void reset() {
    set_text_attributes(default_attributes_);
    set_cursor_visible(true);
  }

 private:

  static HANDLE get_stdout_handle() {
    return GetStdHandle(STD_OUTPUT_HANDLE);
  }

  static int get_text_attributes() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hstdout = get_stdout_handle();
    GetConsoleScreenBufferInfo(hstdout, &csbi);
    return csbi.wAttributes;
  }

  static void set_text_attributes(int flags) {
    HANDLE hstdout = get_stdout_handle();
    SetConsoleTextAttribute(hstdout, (WORD)flags);
  }
};
#else
class console_handler {

 public:
  void set_foreground_color(const color& color) {

    // flush everything
    std::cout.flush();
    // get current color
    switch (color) {
      case BLACK: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_BLACK();
        break;
      }
      case DARK_RED: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_DARK_RED();
        break;
      }
      case DARK_GREEN: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_DARK_GREEN();
        break;
      }
      case DARK_YELLOW: {
         std::cout << ANSI_CODES::FOREGROUND_COLOR_DARK_YELLOW();
        break;
      }
      case DARK_BLUE: {
         std::cout << ANSI_CODES::FOREGROUND_COLOR_DARK_BLUE();
        break;
      }
      case DARK_MAGENTA: {
         std::cout << ANSI_CODES::FOREGROUND_COLOR_DARK_MAGENTA();
         break;
      }
      case DARK_CYAN: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_DARK_CYAN();
        break;
      }
      case LIGHT_GREY: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_LIGHT_GREY();
        break;
      }
      case DARK_GREY: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_DARK_GREY();
        break;
      }
      case RED: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_RED();
        break;
      }
      case GREEN: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_GREEN();
        break;
      }
      case YELLOW: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_YELLOW();
        break;
      }
      case BLUE: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_BLUE();
        break;
      }
      case MAGENTA: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_MAGENTA();
        break;
      }
      case CYAN: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_CYAN();
        break;
      }
      case WHITE: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_WHITE();
        break;
      }
      case DEFAULT: {
        std::cout << ANSI_CODES::FOREGROUND_COLOR_DEFAULT();
        break;
      }
    }
    std::cout.flush();
  }
  
  void set_background_color(const color& color) {

    // flush everything
    std::cout.flush();

    // get current color
    switch (color) {
      case BLACK: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_BLACK();
        break;
      }
      case DARK_RED: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_DARK_RED();
        break;
      }
      case DARK_GREEN: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_DARK_GREEN();
        break;
      }
      case DARK_YELLOW: {
         std::cout << ANSI_CODES::BACKGROUND_COLOR_DARK_YELLOW();
        break;
      }
      case DARK_BLUE: {
         std::cout << ANSI_CODES::BACKGROUND_COLOR_DARK_BLUE();
        break;
      }
      case DARK_MAGENTA: {
         std::cout << ANSI_CODES::BACKGROUND_COLOR_DARK_MAGENTA();
         break;
      }
      case DARK_CYAN: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_DARK_CYAN();
        break;
      }
      case LIGHT_GREY: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_LIGHT_GREY();
        break;
      }
      case DARK_GREY: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_DARK_GREY();
        break;
      }
      case RED: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_RED();
        break;
      }
      case GREEN: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_GREEN();
        break;
      }
      case YELLOW: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_YELLOW();
        break;
      }
      case BLUE: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_BLUE();
        break;
      }
      case MAGENTA: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_MAGENTA();
        break;
      }
      case CYAN: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_CYAN();
        break;
      }
      case WHITE: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_WHITE();
        break;
      }
      case DEFAULT: {
        std::cout << ANSI_CODES::BACKGROUND_COLOR_DEFAULT();
        break;
      }
    }
    std::cout.flush();
  }

  void set_colors_reverse(bool set) {
    std::cout.flush();
    if (set) {
      std::cout << ANSI_CODES::COLOR_REVERSE();
    } else {
      std::cout << ANSI_CODES::COLOR_UNREVERSE();
    }
    std::cout.flush();
  }

  void reset_colors() {
    std::cout.flush();
    std::cout << ANSI_CODES::COLOR_RESET();
    std::cout.flush();
  }

  void set_cursor_position(int r, int c) {
    if (r <= 0) {
      r = 0;
    }
    if (c <= 0) {
      c = 0;
    }
    // ANSI is 1-based indexing
    ++r;
    ++c;
    std::cout.flush();
	
	// compute size of necessary string
    size_t size = std::snprintf( nullptr, 0, ANSI_CODES::CURSOR_POSITION().c_str(), r, c) + 1;  // added 1 for terminating \0
    
	// create a new character buffer
    char* buf = new char[size];
    std::snprintf( buf, size, ANSI_CODES::CURSOR_POSITION().c_str(), r, c); // actually do the snprintf
    std::string msg( buf, buf + size - 1 );      // create output string

	std::cout.flush();
    std::cout << msg;
    std::cout.flush();

	delete[] buf;
  }

  void clear_display() {
    std::cout.flush();
    std::cout << ANSI_CODES::ERASE_DISPLAY();
    std::cout.flush();
  }

  void clear_line() {
    std::cout.flush();
    std::cout << ANSI_CODES::ERASE_LINE();
    std::cout.flush();
  }

  void clear_line_right() {
    std::cout.flush();
    std::cout << ANSI_CODES::ERASE_LINE_TO_END();
    std::cout.flush();
  }

  void clear_line_left() {
    std::cout.flush();
    std::cout << ANSI_CODES::ERASE_LINE_TO_BEGINNING();
    std::cout.flush();
  }
  
  void set_cursor_visible(bool set) {
    std::cout.flush();
    if (set) {
      std::cout << ANSI_CODES::SHOW_CURSOR();
    } else {
      std::cout << ANSI_CODES::HIDE_CURSOR();
    }
    std::cout.flush();
  }

  void reset() {
    std::cout.flush();
    std::cout << ANSI_CODES::COLOR_RESET();
    std::cout.flush();
    set_cursor_visible(true);

  }
};
#endif

} // detail

/**
 * @brief Methods for manipulating the console
 *
 * Useful for cursor placement and visibility, foreground and background colors, and clearing
 * part or all of the screen.
 */
class console {
 public:

  /**
   * @brief Default constructor
   */
  console() : handler_(), foreground_(color::DEFAULT), background_(color::DEFAULT), reversed_(false) {}

  /**
   * @brief Destructor, currently does nothing
   *
   * Note that once the console instance is destructed, any modified console attributes
   * will remain in effect.  It is <em>highly</em> recommended to call `clear_all()` prior
   * to terminating your program.
   */
  virtual ~console() {}

  /**
   * @brief Sets the foreground colour
   *
   * Sets the color of the foreground text, unless colours are set to "reverse", in which case
   * would set the text background color.
   *
   * @param color color to use for foreground
   */
  void set_foreground_color(const color &color) {
    handler_.set_foreground_color(color);
    foreground_ = color;
  }

  /**
   * @brief Sets the background colour
   *
   * Sets the color of the text background, unless colours are set to "reverse", in which case
   * would set the text foreground color.
   * @param color color to use for background
   */
  void set_background_color(const color &color) {
    handler_.set_background_color(color);
    background_ = color;
  }

  /**
   * @brief Reverse role of foreground/background colours
   *
   * If set is `true`, then the foreground colour acts as the background colour, and vice versa.
   * @param set enable or disable reversing of foreground/background colors
   */
  void set_colors_reverse(bool set) {
    handler_.set_colors_reverse(set);
    reversed_ = set;
  }

  /**
   * @brief Reset colours to original values
   *
   * Resets the foreground and background colours to those encountered when this console instance was
   * constructed.
   */
  void reset_colors() {
    handler_.reset_colors();
    foreground_ = DEFAULT;
    background_ = DEFAULT;
    reversed_ = false;
  }

  /**
   * @brief Sets the cursor position
   *
   * The cursor position is the location relative to the top-left corner of the console at which
   * the next printed text is to appear.
   * @param r row, counted from the top which has index `r=0`
   * @param c column, counted from the left which has index `c=0`
   */
  void set_cursor_position(int r, int c) {
    handler_.set_cursor_position(r,c);
  }

  /**
   * @brief Clears contents visible in the console
   *
   * Clears all text and fills the console background with the currently set background colour.
   */
  void clear_display() {
    handler_.clear_display();
  }

  /**
   * @brief Clears the current console line
   *
   * Clears all text and fills the current console line's background with the currently set
   * background colour.
   */
  void clear_line() {
    handler_.clear_line();
  }

  /**
   * @brief Clears contents of the current row to the right of the cursor's position
   *
   * Clears text in the starting with and including the current cursor position until the end of
   * the row, filling with the currently set background colour.
   */
  void clear_line_right() {
    handler_.clear_line_right();
  }

  /**
   * @brief Clears contents of the current row to the left of the cursor's position
   *
   * Clears all text starting with the beginning of the current row and ending with and including
   * the current cursor position, filling with the currently set background colour.
   */
  void clear_line_left() {
    handler_.clear_line_left();
  }

  /**
   * @brief Show or hide the cursor
   * @param visible if `true`, the cursor will be set visible, otherwise the cursor will be hidden
   */
  void set_cursor_visible(bool visible) {
    handler_.set_cursor_visible(visible);
  }

  /**
   * @brief Reset colours and cursor visibility
   *
   * Resets the foreground and background colour to their original values, and re-enables
   * cursor visibility if it was set to hidden
   */
  void reset() {
    handler_.reset();
  }

  /**
   * @brief Clears display and resets all console attributes
   *
   * Resets all console attributes, including foreground and background colours as well as
   * cursor visibility, and clears the contents of the entire display.  The cursor's position
   * is set to the top-left position.  This method should be called prior to program termination
   * to prevent modified console attributes from persisting beyond the lifetime of the process.
   */
  void clear_all() {
    handler_.reset();
    handler_.clear_display();
    handler_.set_cursor_position(0, 0);
  }

 private:
  detail::console_handler handler_;
  color foreground_;
  color background_;
  bool reversed_;
};

} // cpen333

#endif //CPEN333_CONSOLE_H
