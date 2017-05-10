#ifndef RADIANT_TRACE_STD_FILTER_HPP
#define RADIANT_TRACE_STD_FILTER_HPP

#include "Trace.hpp"

namespace Radiant
{
  namespace Trace
  {
    /// Trace Filter that outputs the message to stdout/stderr or optionally
    /// an user given file.
    class RADIANT_API StdFilter : public Filter
    {
    public:
      StdFilter();
      virtual ~StdFilter();

      /// Prints message to the output, always returns false
      bool trace(const Message & msg) override;

      /// Forces ANSI colors to the output even if the output isn't ANSI-capable terminal
      /// @param force Are the colors forced.
      inline void setForceColors(bool force) { m_forceColors = force; }
      inline bool forceColors() const { return m_forceColors; }

      /// Toggle thread id printing
      /// If enabled, each log line will include a unique thread id
      /// @param enabled toggle id printing
      inline void setPrintThreadId(bool enabled) { m_printThreadId = enabled; }
      inline bool printThreadId() const { return m_printThreadId; }

      /// Sets the application name to be used in debug output.
      ///
      /// Each output line will begin with the application name. This is handy
      /// if there are several applications throwing output to the same
      /// terminal window, and you want to know which application is
      /// responsible for which output.
      /// @param appname application name
      inline void setApplicationName(const QByteArray & applicationName) { m_applicationName = applicationName; }
      inline QByteArray applicationName() const { return m_applicationName; }

      /// @param filename If non-empty, this file will be used as the output
      ///        target for all debug/error output instead of stdout / stderr
      void setTraceFile(const QString & filename);
      inline QString traceFile() const { return m_traceFile; }

    private:
      FILE * m_outFile = nullptr;
      QString m_traceFile;
      bool m_forceColors = false;
      bool m_stdoutIsTty = false;
      bool m_stderrIsTty = false;
      bool m_printThreadId = false;
      QByteArray m_applicationName;
    };
  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_STD_FILTER_HPP
