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
#include <set>

namespace Radiant {

  static Radiant::MutexStatic g_mutex;

  static bool g_enableVerboseOutput = false;
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

  static void g_output(Severity s, const char * module, const char * msg, va_list& args)
  {
    FILE * out = (s > WARNING) ? stderr : stdout;

    if(__outfile)
      out = __outfile;

    Radiant::TimeStamp now = Radiant::TimeStamp::getTime();

    g_mutex.lock();
    if(g_appname.empty()) {
      if (module) {
        fprintf(out, "[%s] %s> %s", now.asString().c_str(), module, prefixes[s]);
      } else {
        fprintf(out, "[%s] %s", now.asString().c_str(), prefixes[s]);
      }
    } else {
      if (module) {
        fprintf(out, "[%s] %s: %s> %s", now.asString().c_str(), g_appname.c_str(), module, prefixes[s]);
      } else {
        fprintf(out, "[%s] %s: %s", now.asString().c_str(), g_appname.c_str(), prefixes[s]);
      }
    }

    vfprintf(out, msg, args);
    fprintf(out,"\n");
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
