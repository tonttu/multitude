/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */


#ifndef RADIANT_TRACE_HPP
#define RADIANT_TRACE_HPP

#include <Radiant/Export.hpp>

#include <QString>

#define FNAME static const char * fname = __FUNCTION__

namespace Radiant {

  /// Error severity levels
  enum Severity
  {
    /// Debug information, that is usually not useful for the end user
    /** Debug mesages are printed out only if verbose output is
    enabled. */
    DEBUG,
    /// Useful information to all users.
    /** Info messages are printed out always. */
    INFO,
    WARNING,
    /// An error occurred
    FAILURE,
    /// Fatal error, causes application shutdown
    FATAL
  };

#ifdef __GNUC__
#define RADIANT_PRINTF_CHECK(STR_IDX, FIRST_TO_CHECK) \
  __attribute__ ((format (printf, (STR_IDX), (FIRST_TO_CHECK))))
#else
#define RADIANT_PRINTF_CHECK(STR_IDX, FIRST_TO_CHECK)
#endif

  /// Display useful output.
  /** This function prints out given message, based on current verbosity level.

      Radiant includes a series of functions to write debug output on the
      terminal.

      The functions #info, #debug, #error and #fatal print output to the
      sceen in standardized format. The debug function only writes data to
      the screen if verbose reporting is enabled with #enableVerboseOutput. These functions are
      basically wrappers around printf.

      The terminal output is protected by mutex lock so that multiple
      threads can write to the same terminal without producing corrupted
      output. This was also the reason why the output is done with
      functions, rather than than std::cout etc. With the std streams one
      cannot organize a mutex lock around the text output, which easily
      results in corrupted (and rather useless) output.

      @param s severity of the message
      @param msg message format string */
  RADIANT_API void trace(Severity s, const char * msg, ...) RADIANT_PRINTF_CHECK(2, 3);

  /// @copydoc trace
  RADIANT_API void traceMsg(Severity s, const char * msg);

  /// @copydoc trace
  /// @param module outputting module from which this message originates
  RADIANT_API void trace
  (const char * module, Severity s, const char * msg, ...) RADIANT_PRINTF_CHECK(3, 4);

  /// Display debug output
  /** This function calls trace to do the final work and it is
      effectively the same as calling trace(DEBUG, ...).

      @param msg message
      @see trace
  */
  RADIANT_API void debug(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);
  /// Display information output
  /** This function calls trace to do the final work and it is
      effectively the same as calling trace(INFO, ...).

      @param msg message
      @see trace
  */
  RADIANT_API void info(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);
  /// Display error output
  /** This function calls trace to do the final work and it is
      effectively the same as calling trace(FAILURE, ...).

      @param msg message
      @see trace
  */
  RADIANT_API void error(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);

  /// Display error output, with a fatal message
  /** This function calls trace to do the final work and it is
      effectively the same as calling trace(FATAL, ...).
      @param msg message
      @see trace
  */
  RADIANT_API void fatal(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);

  /// Display error output, with a warning message
  /** This function calls trace to do the final work and it is
      effectively the same as calling trace(WARNING, ...).
      @param msg message
      @see trace
  */
  RADIANT_API void warning(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);

  /** Toggle verbose output.

      If enabled, messages sent with the #debug function are displayed
      to the user. Otherwise they are silently ignored

      @param enable enable or disable messages
      @param module if given, enables or disables verbose output only for given module.
  */
  RADIANT_API void enableVerboseOutput(bool enable, const QString & module = QString());
  /// Returns true if the #debug function output is displayed
  RADIANT_API bool enabledVerboseOutput();
  /// Forces ANSI colors to the output even if the output isn't ANSI-capable terminal
  RADIANT_API void forceColors(bool enable = true);


  /// Toggle duplicate filter
  /// If enabled, duplicate messages will be ignored
  /// @param enable toggle filtering
  RADIANT_API void enableDuplicateFilter(bool enable);

  /// Returns true if the duplicate filter is enabled
  /// @return true if filtering is enabled
  RADIANT_API bool enabledDuplicateFilter();

  /// Toggle thread id printing
  /// If enabled, each log line will include a unique thread id
  /// @param enable toggle id printing
  RADIANT_API void enableThreadId(bool enable);

  /// Returns true if the thread id printing is enabled
  /// @return true if thread id printing is enabled
  RADIANT_API bool enabledThreadId();

  /** Sets the application name to be used in debug output.

      By default the info/debug/error functions will print out the
      error message, without further information. You can set the
      application name with this function, and once this is done each
      output line will begin with the application name. This is handy
      if there are several applications throwing output to the same
      terminal window, and you want to know which application is
      responsible for which output.
      @param appname application name
   */
  RADIANT_API void setApplicationName(const char * appname);

  /// Uses the given file as the output target for all debug/error output.
  /// @param filename output filename
  RADIANT_API void setTraceFile(const char * filename);

}

#endif
