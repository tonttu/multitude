/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_LOG_HPP
#define RADIANT_LOG_HPP

#include "Export.hpp"

namespace Radiant {

  /** Provides logging sevices for the application. The logging system
      is thread-safe, and it is meant for applications to store usage
      information.

      The log mechanism is is not meant for debug output or error
      messages. For those purposes, see Radiant::info, and
      Radiant::error.*/
  class RADIANT_API Log
  {
  public:

    /** Sets the log file.

    @param logfile log filename
    @return This function true if the the new log file was opened
    successfully, and false if opening failed.
     */
    static bool setLogFile(const char * logfile);
    /** Open a log file, with opening time-stamp as part of the file-name.

        Calling setTimedLogFile("foo") will open log file
        foo-2010-01-04T12-45-12-log.txt, where the numbers present the time
        in the ISO format.
        @param prefix log file prefix
        @return true on success
    */
    static bool setTimedLogFile(const char * prefix);

    /** Saves a log message to the file. When forming the log messages, it makes
        sense to have them formatted so that it is easier to parse them automatically
        later on.
        @param str message */
    static void log(const char * str, ...);

  private:
    Log() {}
    Log(const Log &) {}
  };

}

#endif
