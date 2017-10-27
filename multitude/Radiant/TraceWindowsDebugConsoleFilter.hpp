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
