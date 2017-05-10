#include "TraceWindowsDebugConsoleFilter.hpp"

#include <Windows.h>

namespace Radiant
{
  namespace Trace
  {
    WindowsDebugConsoleFilter::WindowsDebugConsoleFilter()
      : Filter(ORDER_OUTPUT)
    {}

    bool WindowsDebugConsoleFilter::trace(const Message & message)
    {
      if (message.module.isEmpty()) {
        OutputDebugStringA((message.msg.toUtf8() + "\n").data());
      } else {
        OutputDebugStringA((message.module + ": " + message.msg.toUtf8() + "\n").data());
      }
      return false;
    }

  } // namespace Trace
} // namespace Radiant
