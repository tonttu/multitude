#include "TraceSeverityFilter.hpp"

namespace Radiant
{
  namespace Trace
  {
    SeverityFilter::SeverityFilter()
      : Filter(ORDER_DEFAULT_FILTERS)
    {}

    void SeverityFilter::setMinimumSeverityLevel(Severity s)
    {
      m_minimumSeverityLevel = s;
    }

    bool SeverityFilter::trace(const Message & msg)
    {
      return msg.severity < m_minimumSeverityLevel &&
          (msg.module.isEmpty() || m_verboseModules.count(msg.module) == 0);
    }

    void SeverityFilter::setVerboseModule(const QByteArray & module, bool verbose)
    {
      if (verbose) {
        m_verboseModules.insert(module);
      } else {
        m_verboseModules.erase(module);
      }
    }

    void SeverityFilter::setVerboseModules(std::set<QByteArray> modules)
    {
      m_verboseModules = modules;
    }

    const std::set<QByteArray> & SeverityFilter::verboseModules() const
    {
      return m_verboseModules;
    }

    bool SeverityFilter::isVerbose(const QByteArray & module) const
    {
      return m_minimumSeverityLevel == DEBUG || module.isEmpty() ||
          m_verboseModules.count(module) > 0;
    }
  } // namespace Trace
} // namespace Radiant
