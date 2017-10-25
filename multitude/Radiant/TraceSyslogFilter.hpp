#ifndef RADIANT_TRACE_SYSLOG_FILTER_HPP
#define RADIANT_TRACE_SYSLOG_FILTER_HPP

#include "Trace.hpp"

namespace Radiant
{
  namespace Trace
  {
    class RADIANT_API SyslogFilter : public Filter
    {
    public:
      SyslogFilter(const QByteArray & ident, Severity minSeverity);
      virtual ~SyslogFilter();

      bool trace(Message & message) override;

    private:
      QByteArray m_ident;
      Severity m_minSeverity;
    };
  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_SYSLOG_FILTER_HPP
