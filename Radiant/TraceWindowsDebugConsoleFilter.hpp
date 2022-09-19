/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef TRACE_WINDOWS_DEBUG_CONSOLE_FILTER_HPP
#define TRACE_WINDOWS_DEBUG_CONSOLE_FILTER_HPP

#include "Trace.hpp"

namespace Radiant
{
  namespace Trace
  {
    class WindowsDebugConsoleFilter : public Filter
    {
    public:
      WindowsDebugConsoleFilter();

      bool trace(Message & message) override;
    };

  } // namespace Trace
} // namespace Radiant

#endif // TRACE_WINDOWS_DEBUG_CONSOLE_FILTER_HPP
