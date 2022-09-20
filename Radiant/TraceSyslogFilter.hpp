/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
