/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
      bool trace(Message & msg) override;

      void setMinimumSeverityLevel(Severity s);
      void setVerboseModule(const QByteArray & module, bool verbose);
      void setVerboseModules(std::set<QByteArray> modules);

      const std::set<QByteArray> & verboseModules() const;

      /// Returns true if m_minimumSeverityLevel is DEBUG or if the given module
      /// is included in verboseModules
      bool isVerbose(const QByteArray & module) const;

    private:
      Severity m_minimumSeverityLevel = INFO;
      std::set<QByteArray> m_verboseModules;
    };

  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_SEVERITY_FILTER_HPP
