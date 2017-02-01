/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Trace.hpp"
#include "Mutex.hpp"
#include "TimeStamp.hpp"
#include "Platform.hpp"
#include "Thread.hpp"

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

#include <QString>
#include <set>
#include <ctime>

#ifdef RADIANT_LINUX
#include <syslog.h>
#endif

#ifndef RADIANT_WINDOWS
#include <unistd.h> // for istty
#else
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif


namespace
{
#ifdef RADIANT_LINUX
  static int syslogPriority(Radiant::Severity severity)
  {
    switch (severity) {
    case Radiant::FATAL:
      return LOG_ALERT;
    case Radiant::FAILURE:
      return LOG_ERR;
    case Radiant::WARNING:
      return LOG_WARNING;
    case Radiant::INFO:
      return LOG_INFO;
    case Radiant::DEBUG:
    default:
      return LOG_DEBUG;
    }
  }
#endif
}

namespace Radiant {

  static Radiant::Mutex g_mutex;

  static bool g_enableVerboseOutput = false;
  static bool g_enableDuplicateFilter = false;
  static std::string g_lastLogLine = "";
  static bool g_forceColors = false;
  static bool g_enableThreadId = false;
  static std::set<QString> g_verboseModules;
#ifdef RADIANT_LINUX
  static int s_syslogMinSeverity = -1;
#endif
  static QByteArray s_syslogIdent;
  static Severity s_minimumSeverityLevel = INFO;

  QString g_appname;

  static FILE * __outfile = 0;

  const char * prefixes[] = {
    "[DEBUG] ",
    "",
    "[WARNING] ",
    "[ERROR] ",
    "[FATAL] "
  };

  void enableDuplicateFilter(bool enable)
  {
    g_enableDuplicateFilter = enable;
  }

  bool enabledDuplicateFilter()
  {
    return g_enableDuplicateFilter;
  }

  void enableThreadId(bool enable)
  {
    g_enableThreadId = enable;
  }

  bool enabledThreadId()
  {
    return g_enableThreadId;
  }

  void setMinimumSeverityLevel(Severity s)
  {
    s_minimumSeverityLevel = s;
  }

  void enableVerboseOutput(bool enable, const QString & module)
  {

    if (!module.isEmpty()) {
      if (enable) {
        g_verboseModules.insert(module);
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
    if(s < s_minimumSeverityLevel)
      return;

    static bool stderr_is_tty = false, stdout_is_tty = false;

#ifndef RADIANT_WINDOWS
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

    const char * timestamp_color = "";
    const char * color = "";
    const char * colors_end = "";

#ifndef RADIANT_OSX
    bool use_colors = g_forceColors || (out == stderr && stderr_is_tty) || (out == stdout && stdout_is_tty);

    /* On OSX Mountain Lion the color switching code seems to corrupt the terminal
       if the application crashes. It is easier to just not use the colors. */
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
#endif

    char storage[1024];
    char * buffer = storage;
    int size = sizeof(storage), ret = 0;

    if(g_enableThreadId) {
      static RADIANT_TLS(int) t_id(-1);
      int& id = t_id;
      static int s_id = 0;
      if(id < 0) {
        Guard lock(g_mutex);
        id = s_id++;
      }
      ret = snprintf(buffer, size, "%3d ", id);
      if(ret > 0) size -= ret, buffer += ret;
    }

    if(g_appname.isEmpty()) {
      if(module) {
        ret = snprintf(buffer, size, "%s> %s%s", module, color, prefixes[s]);
      } else {
        ret = snprintf(buffer, size, "%s%s", color, prefixes[s]);
      }
    } else {
      if (module) {
        ret = snprintf(buffer, size, "%s: %s> %s%s", g_appname.toUtf8().data(), module, color, prefixes[s]);
      } else {
        ret = snprintf(buffer, size, "%s: %s%s", g_appname.toUtf8().data(), color, prefixes[s]);
      }
    }
    if(ret > 0) size -= ret, buffer += ret;

    vsnprintf(buffer, size, msg, args);

    Radiant::TimeStamp now = Radiant::TimeStamp::currentTime();

    // Skip duplicates
    if(enabledDuplicateFilter()) {
      Guard lock(g_mutex);
      std::string tmp(storage);
      if(g_lastLogLine == tmp)
        return;

      g_lastLogLine = tmp;
    }

    time_t t = now.value() >> 24;
    /// localtime is not thread-safe on unix, and localtime_r isn't defined in windows
#ifdef RADIANT_WINDOWS
    struct tm * ts = localtime(&t);
#else
    struct tm tmp;
    struct tm * ts = localtime_r(&t, &tmp);
#endif

    fprintf(out, "%s[%04d-%02d-%02d %02d:%02d:%02d.%03d]%s %s%s\n", timestamp_color,
            ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            int(now.subSecondsUS()) / 1000, colors_end, storage,
            colors_end);

#ifdef RADIANT_LINUX
    if (s_syslogMinSeverity != -1 && s >= s_syslogMinSeverity) {
      // In Linux we cant call vsyslog with msg and args without first copying
      // args with va_copy. It's easier to just use pre-formatted buffer instead
      syslog(syslogPriority(s), "%s", buffer);
    }
#endif

#ifdef _WIN32
    // Log to the Windows debug-console as well
    char logmsg[256];
    int len = _snprintf(logmsg, 16, "[thr:%d] ", GetCurrentThreadId());

    int len2 = vsnprintf(logmsg+len, 256-len, msg, args);
    if (len + len2 > 254 || len2 < 0) {
      logmsg[254] = '\n';
      logmsg[255] = '\0';
    } else {
      logmsg[len+len2] = '\n';
      logmsg[len+len2+1] = '\0';
    }

    OutputDebugStringA(logmsg);
#endif
    fflush(out);
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

  void traceMsg(Severity s, const char * msg)
  {
    trace(s, "%s", msg);
  }

  bool isVerbose(const char * module)
  {
    return g_enableVerboseOutput || (module && g_verboseModules.count(module) > 0);
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

    abort();

    exit(EXIT_FAILURE);
  }

  void setApplicationName(const char * appname)
  {
    g_appname = appname;
  }

  void setTraceFile(const char * filename)
  {
    FILE * tmp = fopen(filename, "w");
    if(!tmp) {
      error("Radiant::setTraceFile # Failed to open %s", filename);
    } else {
      Guard lock(g_mutex);
      if(__outfile)
        fclose(__outfile);
      __outfile = tmp;
      printf("Trace file set to %s (%p)\n", filename, __outfile);
      fflush(0);
    }
  }

#ifdef RADIANT_LINUX
  void openSyslog(const QString & ident, Severity minSeverity)
  {
    if (s_syslogMinSeverity == minSeverity && s_syslogIdent == ident.toUtf8())
      return;

    if (s_syslogMinSeverity != -1)
      closeSyslog();

    // GNU openlog doesn't make a copy of the ident, so we need to save it
    s_syslogIdent = ident.toUtf8();
    openlog(s_syslogIdent.data(), LOG_NDELAY, LOG_USER);
    s_syslogMinSeverity = minSeverity;
  }

  void closeSyslog()
  {
    if (s_syslogMinSeverity != -1) {
      s_syslogMinSeverity = -1;
      closelog();
      s_syslogIdent.clear();
    }
  }
#endif
}
