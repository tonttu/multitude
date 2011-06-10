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
#include "Platform.hpp"

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <string>
#include <set>

#ifndef RADIANT_WIN32
#include <unistd.h> // for istty
#endif

namespace Radiant {

  static Radiant::Mutex g_mutex;

  static bool g_enableVerboseOutput = false;
  static bool g_forceColors = false;
  static std::set<std::string> g_verboseModules;

  std::string g_appname;

  static FILE * __outfile = 0;

  const char * prefixes[] = {
    "[DEBUG] ",
    "",
    "[WARNING] ",
    "[ERROR] ",
    "[FATAL] "
  };

  void enableVerboseOutput(bool enable, const char * module)
  {

    if (module) {
      if (enable) {
        g_verboseModules.insert(std::string(module));
      } else {
        g_verboseModules.erase(module);
      }
    } else {
      g_enableVerboseOutput = enable;
    }
  }

  bool enabledVerboseOutput()
  {
    return g_enableVerboseOutput;
  }

  void forceColors(bool enable)
  {
    g_forceColors = enable;
  }

  static void g_output(Severity s, const char * module, const char * msg, va_list& args)
  {
    static bool stderr_is_tty = false, stdout_is_tty = false;

#ifndef RADIANT_WIN32
    // this doesn't need mutex, it doesn't matter if this is ran
    // in two different threads at the same time
    static bool once = false;
    if (!once) {
      const char* term = getenv("TERM");
      if(term && term != std::string("dumb")) {
        stderr_is_tty = isatty(2);
        stdout_is_tty = isatty(1);
      }
      once = true;
    }
#endif

    FILE * out = (s > WARNING) ? stderr : stdout;

    if(__outfile)
      out = __outfile;

    bool use_colors = g_forceColors || (out == stderr && stderr_is_tty) ||
        (out == stdout && stdout_is_tty);

    const char* timestamp_color = "";
    const char* color = "";
    const char* colors_end = "";

    if(use_colors) {
      if(s == WARNING) {
        color = "\033[1;33m";
      } else if(s > WARNING) {
        color = "\033[1;31m";
      } else if(s == DEBUG) {
        color = "\033[35m";
      }
      timestamp_color = "\033[1;30m";
      colors_end = "\033[0m";
    }

    Radiant::TimeStamp now = Radiant::TimeStamp::getTime();

    g_mutex.lock();

    time_t t = now.value() >> 24;
    /// localtime is not thread-safe
    struct tm * ts = localtime(&t);
    fprintf(out, "%s[%04d-%02d-%02d %02d:%02d:%02d.%03d]%s ", timestamp_color,
            ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec, int(now.subSecondsUS()) / 1000,
            colors_end);

    if(g_appname.empty()) {
      if (module) {
        fprintf(out, "%s> %s%s", module, color, prefixes[s]);
      } else {
        fprintf(out, "%s%s", color, prefixes[s]);
      }
    } else {
      if (module) {
        fprintf(out, "%s: %s> %s%s", g_appname.c_str(), module, color, prefixes[s]);
      } else {
        fprintf(out, "%s: %s%s", g_appname.c_str(), color, prefixes[s]);
      }
    }

    vfprintf(out, msg, args);
    fprintf(out,"%s\n", colors_end);
    fflush(out);
    g_mutex.unlock();
  }

  void trace(Severity s, const char * msg, ...)
  {
    if(g_enableVerboseOutput || s > DEBUG) {

      va_list args;
      va_start(args, msg);
      g_output(s, 0, msg, args);
      va_end(args);

      if(s >= FATAL) {
        exit(0);
        _exit(0);
        // Sometimes "exit" is not enough, this is guaranteed to work
        int * bad = 0;
        *bad = 123456;
      }
    }
  }


  void trace(const char * module, Severity s, const char * msg, ...)
  {
    if (s > DEBUG || (g_enableVerboseOutput || (module && g_verboseModules.count(module) > 0))) {
      va_list args;
      va_start(args, msg);
      g_output(s, module, msg, args);
      va_end(args);

      if(s >= FATAL) {
        exit(0);
        _exit(0);
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

    va_list args;
    va_start(args, msg);
    g_output(DEBUG, 0, msg, args);
    va_end(args);
  }

  void info(const char * msg, ...)
  {
    va_list args;
    va_start(args, msg);
    g_output(INFO, 0, msg, args);
    va_end(args);
  }

  void warning(const char * msg, ...)
  {
    va_list args;
    va_start(args, msg);
    g_output(WARNING, 0, msg, args);
    va_end(args);
  }

  void error(const char * msg, ...)
  {
    va_list args;
    va_start(args, msg);
    g_output(FAILURE, 0, msg, args);
    va_end(args);
  }

  void fatal(const char * msg, ...)
  {
    va_list args;
    va_start(args, msg);
    g_output(FATAL, 0, msg, args);
    va_end(args);

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
