/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "PlatformUtils.hpp"
#include "Trace.hpp"
#include "TraceSeverityFilter.hpp"
#include "TraceStdFilter.hpp"

#include <Radiant/CallStack.hpp>

#ifdef RADIANT_UNIX
#include <unistd.h>
#endif

#include <vector>

namespace Radiant
{
  namespace Trace
  {
    namespace
    {
      static std::multimap<float, Radiant::Trace::FilterPtr> s_filters;
      static std::vector<Message> s_queue;
      static bool s_initialized = false;

      void qtMessageHandler(QtMsgType type, const QMessageLogContext & ctx, const QString & msg)
      {
        // By default Qt includes file/line/func in debug builds but not in
        // release builds. To avoid extra string manipulation, just have two
        // switches here.
        if (ctx.file) {
          const char * file = ctx.file ? ctx.file : "";
          const char * func = ctx.function ? ctx.function : "";
          const QByteArray & str = msg.toUtf8();
          switch (type) {
            case QtDebugMsg:
              trace("Qt", DEBUG, "%s:%d [%s]: %s", file, ctx.line, func, str.data());
              break;

            case QtInfoMsg:
              trace("Qt", INFO, "%s:%d [%s]: %s", file, ctx.line, func, str.data());
              break;

            case QtWarningMsg:
              trace("Qt", WARNING, "%s:%d [%s]: %s", file, ctx.line, func, str.data());
              break;

            case QtCriticalMsg:
              trace("Qt", FAILURE, "%s:%d [%s]: %s", file, ctx.line, func, str.data());
              break;

            case QtFatalMsg:
              trace("Qt", FATAL, "%s:%d [%s]: %s", file, ctx.line, func, str.data());
              break;
          }
        } else {
          switch (type) {
            case QtDebugMsg:
              traceMsg("Qt", DEBUG, msg);
              break;

            case QtInfoMsg:
              traceMsg("Qt", INFO, msg);
              break;

            case QtWarningMsg:
              traceMsg("Qt", WARNING, msg);
              break;

            case QtCriticalMsg:
              traceMsg("Qt", FAILURE, msg);
              break;

            case QtFatalMsg:
              traceMsg("Qt", FATAL, msg);
              break;
          }
        }

        // Trying to find what causes https://redmine.multitaction.com/issues/16343
        if (msg.contains("Timers cannot be stopped"))
          Radiant::CallStack().print();
      }
    }

    ///////////////////////////////////////////////////////////////////////////

    struct LambdaFilter : public Trace::Filter
    {
      LambdaFilter(const Trace::FilterFunc & func, float order)
        : Filter(order)
        , m_func(func)
      {}

      bool trace(Message & msg) override
      {
        return m_func(msg);
      }

      const Trace::FilterFunc m_func;
    };

    ///////////////////////////////////////////////////////////////////////////

    static void processFilters(Message & msg)
    {
      for (auto & p: s_filters) {
        Filter & filter = *p.second;
        if (filter.trace(msg)) {
          break;
        }
      }
    }

    static inline void processMessage(Message & msg)
    {
      if (s_initialized) {
        processFilters(msg);
        return;
      }

      if (s_queue.empty()) {
        // If we close the application (or crash) before the system has been
        // initialized, try to not lose the messages and just print them on
        // stderr
        atexit([] {
          if (!s_initialized) {
            bool headerPrinted = false;
            for (Message & msg: s_queue) {
              if (msg.severity > DEBUG) {
                if (!headerPrinted) {
                  fprintf(stderr, "%s: application closed before Radiant::Trace was initialized, queued messages:\n",
                          PlatformUtils::getExecutablePath().toUtf8().data());
                  headerPrinted = true;
                }
                fprintf(stderr, "%s %s\n", severityText(msg.severity).data(), msg.text.toUtf8().data());
              }
            }
          }
        });
      }
      s_queue.push_back(std::move(msg));
    }

    static void processMessage(Severity s, QByteArray module, const char * format, va_list & ap)
    {
      Message msg;
      msg.module = std::move(module);
      msg.severity = s;
      msg.text = QString::vasprintf(format, ap);

      processMessage(msg);
    }

    static void crash()
    {
      for (Message & msg: s_queue) {
        fprintf(stderr, "%s\n", msg.text.toUtf8().data());
      }
      abort();
      _exit(1);
    }

    void addFilter(const FilterPtr & filter)
    {
      s_filters.insert(std::make_pair(filter->order(), filter));
    }

    FilterPtr addFilter(const FilterFunc & filterFunc, float order)
    {
      auto filter = std::make_shared<LambdaFilter>(filterFunc, order);
      addFilter(filter);
      return filter;
    }

    bool removeFilter(const FilterPtr & filter)
    {
      for (auto it = s_filters.begin(), end = s_filters.end(); it != end; ++it) {
        if (it->second == filter) {
          s_filters.erase(it);
          return true;
        }
      }
      return false;
    }

    std::multimap<float, FilterPtr> filters()
    {
      return s_filters;
    }

    void initialize(Radiant::FlagsT<InitFlags> flags)
    {
      decltype(s_queue) queue;
      std::swap(s_queue, queue);

      if (flags & INIT_DEFAULT_FILTERS) {
        findOrCreateFilter<SeverityFilter>();
        findOrCreateFilter<StdFilter>();
      }

      s_initialized = true;

      if (flags & INIT_PROCESS_QUEUED_MESSAGES) {
        for (Message & msg: queue) {
          processFilters(msg);
        }
      }

      if (flags & INIT_QT_MESSAGE_HANDLER) {
        qInstallMessageHandler(qtMessageHandler);
      }
    }

    void trace(Severity s, const char * msg, ...)
    {
      va_list args;
      va_start(args, msg);
      processMessage(s, nullptr, msg, args);
      va_end(args);

      if (s == FATAL)
        crash();
    }

    void trace(const char * module, Severity s, const char * msg, ...)
    {
      va_list args;
      va_start(args, msg);
      processMessage(s, module, msg, args);
      va_end(args);

      if (s == FATAL)
        crash();
    }

    void traceMsg(Severity s, const QString & text)
    {
      Message msg;
      msg.severity = s;
      msg.text = text;

      processMessage(msg);
    }

    void traceMsg(const QByteArray & module, Severity s, const QString & text)
    {
      Message msg;
      msg.module = module;
      msg.severity = s;
      msg.text = text;

      processMessage(msg);
    }

    void debug(const char * msg, ...)
    {
      va_list args;
      va_start(args, msg);
      processMessage(DEBUG, nullptr, msg, args);
      va_end(args);
    }

    void info(const char * msg, ...)
    {
      va_list args;
      va_start(args, msg);
      processMessage(INFO, nullptr, msg, args);
      va_end(args);
    }

    void warning(const char * msg, ...)
    {
      va_list args;
      va_start(args, msg);
      processMessage(WARNING, nullptr, msg, args);
      va_end(args);
    }

    void error(const char * msg, ...)
    {
      va_list args;
      va_start(args, msg);
      processMessage(FAILURE, nullptr, msg, args);
      va_end(args);
    }

    void fatal(const char * msg, ...)
    {
      va_list args;
      va_start(args, msg);
      processMessage(FATAL, nullptr, msg, args);
      va_end(args);

      crash();
    }

    QByteArray severityText(Radiant::Trace::Severity severity)
    {
      switch (severity) {
      case DEBUG:
        return "DEBUG";
      case INFO:
        return "INFO";
      case WARNING:
        return "WARNING";
      case FAILURE:
        return "ERROR";
      case FATAL:
        return "FATAL";
      default:
        return "UNKNOWN";
      }
    }

  } // namespace Trace
} // namespace Radiant
