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

#include "Trace.hpp"
#include "Mutex.hpp"
#include "TimeStamp.hpp"

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

#include <string>

namespace Radiant {

  static Radiant::MutexStatic g_mutex;

  static bool g_enableVerboseOutput = false;

  std::string g_appname;

  static FILE * __outfile = 0;

  const char * prefixes[] = {
    "[DEBUG] ",
    "",
    "[WARNING] ",
    "[ERROR] ",
    "[FATAL] "
  };

  void enableVerboseOutput(bool enable)
  {
    g_enableVerboseOutput = enable;
  }

  bool enabledVerboseOutput()
  {
    return g_enableVerboseOutput;
  }

  static void g_output(Severity s, const char * msg)
  {
    FILE * out = (s > WARNING) ? stdout : stderr;

    if(__outfile)
      out = __outfile;

    Radiant::TimeStamp now = Radiant::TimeStamp::getTime();

    g_mutex.lock();
    if(g_appname.empty())
      fprintf(out, "[%s] %s%s\n", now.asString().c_str(), prefixes[s], msg);
    else
      fprintf(out, "[%s] %s: %s%s\n", now.asString().c_str(), g_appname.c_str(), prefixes[s], msg);
    fflush(out);
    g_mutex.unlock();
  }

  void trace(Severity s, const char * msg, ...)
  {
    if(g_enableVerboseOutput || s > DEBUG) {


      char buf[4096];
      va_list args;

      va_start(args, msg);
      vsnprintf(buf, 4096, msg, args);
      va_end(args);

      g_output(s, buf);

      if(s >= FATAL) {
        exit(0);
        // Sometimes "exit" is not enough, this is guaranteed to work
        int * bad = 0;
        *bad = 123456;
      }
    }
  }

  void debug(const char * msg, ...)
  {
    if(!g_enableVerboseOutput)
      return;

    char buf[4096];
    va_list args;

    va_start(args, msg);
    vsnprintf(buf, 4096, msg, args);
    va_end(args);

    g_output(DEBUG, buf);
  }

  void info(const char * msg, ...)
  {
    char buf[4096];
    va_list args;

    va_start(args, msg);
    vsnprintf(buf, 4096, msg, args);
    va_end(args);

    g_output(INFO, buf);
  }

  void error(const char * msg, ...)
  {
    char buf[4096];
    va_list args;

    va_start(args, msg);
    vsnprintf(buf, 4096, msg, args);
    va_end(args);

    g_output(FAILURE, buf);
  }

  void fatal(const char * msg, ...)
  {
    char buf[4096];
    va_list args;

    va_start(args, msg);
    vsnprintf(buf, 4096, msg, args);
    va_end(args);

    g_output(FATAL, buf);

    exit(EXIT_FAILURE);
  }

  void setApplicationName(const char * appname)
  {
    g_appname = appname;
  }

  void setTraceFile(const char * filename)
  {
    __outfile = fopen(filename, "w");
    printf("Trace file set to %s (%p)\n", filename, __outfile);
    fflush(0);
  }

}
