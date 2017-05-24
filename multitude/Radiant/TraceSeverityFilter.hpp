#ifndef RADIANT_TRACE_SEVERITY_FILTER_HPP
#define RADIANT_TRACE_SEVERITY_FILTER_HPP

#include "Trace.hpp"

#include <set>

namespace Radiant
{
  namespace Trace
  {
    /// Trace Filter that drops messages based on their severity
    class RADIANT_API SeverityFilter : public Filter
    {
    public:
      SeverityFilter();
      bool trace(const Message & msg) override;

      void setMinimumSeverityLevel(Severity s);
      void setVerboseModule(const QByteArray & module, bool verbose);
      void setVerboseModules(std::set<QByteArray> modules);

    private:
      Severity m_minimumSeverityLevel = INFO;
      std::set<QByteArray> m_verboseModules;
    };

  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_SEVERITY_FILTER_HPP
