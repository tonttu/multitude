/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_TRACE_CRASH_HANDLER_FILTER_HPP
#define RADIANT_TRACE_CRASH_HANDLER_FILTER_HPP

#include "Trace.hpp"
#include "Mutex.hpp"
#include "CrashHandler.hpp"

#include <vector>

namespace Radiant
{
  namespace Trace
  {
    /// Trace Filter that injects application log to crash reports
    class RADIANT_API CrashHandlerFilter : public Filter
    {
    public:
      CrashHandlerFilter();
      virtual ~CrashHandlerFilter();
      bool trace(Message & msg) override;

    private:
      CrashHandler::AttachmentRingBuffer m_buffer;
    };

  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_CRASH_HANDLER_FILTER_HPP
