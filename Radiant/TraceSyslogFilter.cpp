/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "TraceSyslogFilter.hpp"

#include <syslog.h>

static int syslogPriority(Radiant::Trace::Severity severity)
{
  switch (severity) {
  case Radiant::Trace::FATAL:
    return LOG_ALERT;
  case Radiant::Trace::FAILURE:
    return LOG_ERR;
  case Radiant::Trace::WARNING:
    return LOG_WARNING;
  case Radiant::Trace::INFO:
    return LOG_INFO;
  case Radiant::Trace::DEBUG:
  default:
    return LOG_DEBUG;
  }
}

namespace Radiant
{
  namespace Trace
  {
    SyslogFilter::SyslogFilter(const QByteArray & ident, Severity minSeverity)
      : Filter(ORDER_OUTPUT)
      // GNU openlog doesn't make a copy of the ident, so we need to save it
      , m_ident(ident)
      , m_minSeverity(minSeverity)
    {
      openlog(m_ident.data(), LOG_NDELAY, LOG_USER);
    }

    SyslogFilter::~SyslogFilter()
    {
      closelog();
    }

    bool SyslogFilter::trace(Message & message)
    {
      if (message.severity >= m_minSeverity) {
        if (message.module.isEmpty()) {
          syslog(syslogPriority(message.severity), "%s", message.text.toUtf8().data());
        } else {
          syslog(syslogPriority(message.severity), "%s: %s", message.module.data(), message.text.toUtf8().data());
        }
      }

      return false;
    }
  } // namespace Trace
} // namespace Radiant
