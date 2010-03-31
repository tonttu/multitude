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

#define FNAME static const char * fname = __FUNCTION__ 

namespace Radiant {
  
  /*! \page debugoutput Debug/trace output

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

  */
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

  /// Display useful output.
  RADIANT_API void trace(Severity s, const char * msg, ...);
  /// Display debug output
  /** This function calls trace to do the final work and it is
      effectively the same as calling debug(...). */
  RADIANT_API void debug(const char * msg, ...);
  /// Display information output
  /** This function calls trace to do the final work and it is
      effectively the same as calling trace(INFO, ...). */
  RADIANT_API void info(const char * msg, ...);
  /// Display error output
  /** This function calls trace to do the final work and it is
      effectively the same as calling trace(FAILURE, ...). */
  RADIANT_API void error(const char * msg, ...);

  /// Display error output, with a fatal message
  /** This function calls trace to do the final work and it is
      effectively the same as calling trace(FATAL, ...). */
  RADIANT_API void fatal(const char * msg, ...);

  /** Toggle verbose output.

      If enabled, messages sent with the #debug function are displayed
      to the user. Otherwise they are silently ignored
  */
  RADIANT_API void enableVerboseOutput(bool enable);
  RADIANT_API bool enabledVerboseOutput();
  
  /** Sets the application name to be used in debug output. 
      
      By default the info/debug/error functions will print out the
      error message, without further information. You can set the
      application name with this function, and once this is done each
      output line will begin with the application name. This is handy
      if there are several applications throwing output to the same
      terminal window, and you want to know which application is
      responsible for which output.
   */

  RADIANT_API void setApplicationName(const char * appname);

  /** Uses the given file as the output target for all debug/error output. */
  RADIANT_API void setTraceFile(const char * filename);

}

#endif
