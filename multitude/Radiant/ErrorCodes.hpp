fmpeg
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
#ifndef RADIANT_ERROR_ENUMS_HPP
#define RADIANT_ERROR_ENUMS_HPP

namespace Radiant {

  ///
  enum ErrorLevel {
    /// Program execution trace
    ERRL_TRACE,
    ERRL_INFORMATION,
    ERRL_WARNING,
    ERRL_ERROR,
    ERRL_FATAL,
    ERRL_SIZEOF
  };

  extern const char *errorLevelStr(ErrorLevel);

  extern const char *ErrorLevels[ERRL_SIZEOF];
  /*
  /// Error codes.
  enum ErrorCode {
    /// No error
    ERR_NONE,
    /// Uknown error
    ERR_UNKNOWN,
    /// Out of memory
    ERR_MEMORY,
    /// Data corrupted
    ERR_DATA_CORRUPTED,
    /// Internal error
    ERR_INTERNAL,
    /// System error
    ERR_SYSTEM,
    /// Array size exceeded
    ERR_ARRAY_EXCEED,
    /// List size exceeded
    ERR_LIST_EXCEED,
    /// Bad argument
    ERR_ARGUMENT,
    /// Bad argument
    ERR_NULL_ARGUMENT,
    /// Bad match
    ERR_MATCH,
    /// End of file
    ERR_EOF,
    /// Resource denied
    ERR_RC_DENIED,
    /// Only virtual
    ERR_VIRTUAL,
    /// Do not call this function
    ERR_DO_NOT_CALL,
    /// Not implemented yet
    ERR_UNIMPLEMENTED,
    /// Not initialized yet
    ERR_UNINITIALIZED,
    /// Real-time performance failure
    ERR_REALTIME,
    /// Floating point error
    ERR_FPE,
    /// CPU load too high
    ERR_CPU_LOAD,
    /// Numeric overflow
    ERR_OVERFLOW,
    /// Input problem
    ERR_INPUT,
    /// Output problem
    ERR_OUTPUT,
    /// Number of error codes
    ERR_SIZEOF
  };
  */
  extern const char *errorStr(ErrorLevel);

  /// Character strings for error codes:
  // extern const char *ErrorCodes[ERR_SIZEOF];

}

#endif
