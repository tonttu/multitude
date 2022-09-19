/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "TraceWindowsDebugConsoleFilter.hpp"

#include <Windows.h>

namespace Radiant
{
  namespace Trace
  {
    WindowsDebugConsoleFilter::WindowsDebugConsoleFilter()
      : Filter(ORDER_OUTPUT)
    {}

    bool WindowsDebugConsoleFilter::trace(Message & message)
    {
      if (message.module.isEmpty()) {
        OutputDebugStringA((message.text.toUtf8() + "\n").data());
      } else {
        OutputDebugStringA((message.module + ": " + message.text.toUtf8() + "\n").data());
      }
      return false;
    }

  } // namespace Trace
} // namespace Radiant
