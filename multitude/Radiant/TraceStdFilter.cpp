#include "TraceStdFilter.hpp"

#include "Thread.hpp"
#include "FileUtils.hpp"

#include <QFileInfo>
#include <QDir>

#ifndef RADIANT_WINDOWS
#include <unistd.h> // for istty
#endif

#include <atomic>
#include <cstdio>
#include <ctime>

static RADIANT_TLS(int) t_id(-1);
static std::atomic<int> s_threadCounter{0};

static const char * s_prefixes[] = {
  "[DEBUG] ",
  "",
  "[WARNING] ",
  "[ERROR] ",
  "[FATAL] "
};

namespace Radiant
{
  namespace Trace
  {
    StdFilter::StdFilter()
      : Filter(ORDER_OUTPUT)
    {
#ifndef RADIANT_WINDOWS
      const char * term = getenv("TERM");
      if (term && term != QByteArray("dumb")) {
        m_stdoutIsTty = isatty(STDOUT_FILENO);
        m_stderrIsTty = isatty(STDERR_FILENO);
      }
#endif
    }

    StdFilter::~StdFilter()
    {
      if (m_outFile) {
        fclose(m_outFile);
      }
    }

    bool StdFilter::trace(const Message & msg)
    {
      FILE * out = msg.severity > WARNING ? stderr : stdout;

      if (m_outFile)
        out = m_outFile;

      const char * timestampColor = "";
      const char * color = "";
      const char * colorsEnd = "";

      const bool useColors = m_forceColors || (out == stderr && m_stderrIsTty) || (out == stdout && m_stdoutIsTty);

      if (useColors) {
        if (msg.severity == WARNING) {
          color = "\033[1;33m";
        } else if (msg.severity > WARNING) {
          color = "\033[1;31m";
        } else if (msg.severity == DEBUG) {
          color = "\033[35m";
        }
        timestampColor = "\033[1;30m";
        colorsEnd = "\033[0m";
      }

      char storage[128];
      char * buffer = storage;
      int size = sizeof(storage);
      int ret = 0;

      if (m_printThreadId) {
        int & id = t_id;
        if (id < 0) {
          id = s_threadCounter++;
        }
        ret = snprintf(buffer, size, "%3d ", id);
        if (ret > 0) size -= ret, buffer += ret;
      }

      if (m_applicationName.isEmpty()) {
        if (!msg.module.isEmpty()) {
          ret = snprintf(buffer, size, "%s> %s%s", msg.module.data(), color, s_prefixes[msg.severity]);
        } else {
          ret = snprintf(buffer, size, "%s%s", color, s_prefixes[msg.severity]);
        }
      } else {
        if (!msg.module.isEmpty()) {
          ret = snprintf(buffer, size, "%s: %s> %s%s", m_applicationName.data(), msg.module.data(), color, s_prefixes[msg.severity]);
        } else {
          ret = snprintf(buffer, size, "%s: %s%s", m_applicationName.data(), color, s_prefixes[msg.severity]);
        }
      }
      if(ret > 0) size -= ret, buffer += ret;

      const Radiant::TimeStamp now = msg.timestamp();

      time_t t = now.value() >> 24;
      /// localtime is not thread-safe on unix, and localtime_r isn't defined in windows
    #ifdef RADIANT_WINDOWS
      struct tm * ts = localtime(&t);
    #else
      struct tm tmp;
      struct tm * ts = localtime_r(&t, &tmp);
    #endif

      fprintf(out, "%s[%04d-%02d-%02d %02d:%02d:%02d.%03d]%s %s%s%s\n", timestampColor,
              ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday,
              ts->tm_hour, ts->tm_min, ts->tm_sec,
              int(now.subSecondsUS()) / 1000, colorsEnd, storage, msg.text.toUtf8().data(),
              colorsEnd);

      fflush(out);

      return false;
    }

    void StdFilter::setTraceFile(const QString & filename)
    {
      if (m_traceFile == filename) return;
      m_traceFile = filename;

      if (m_outFile) {
        fclose(m_outFile);
        m_outFile = nullptr;
      }

      if (!filename.isEmpty()) {
        QString target = FileUtils::resolvePath(filename);
        if (!QDir().mkpath(QFileInfo(target).path())) {
          error("Radiant::StdFilter::setTraceFile # Failed to create path for %s",
                filename.toUtf8().data());
        }
        m_outFile = fopen(target.toUtf8().data(), "a");
        if (!m_outFile) {
          error("Radiant::StdFilter::setTraceFile # Failed to open %s", filename.toUtf8().data());
          m_traceFile.clear();
        }
      }
    }
  } // namespace Trace
} // namespace Radiant
