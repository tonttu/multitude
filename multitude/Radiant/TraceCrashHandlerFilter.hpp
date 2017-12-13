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
      bool trace(const Message & msg) override;

    private:
      CrashHandler::AttachmentRingBuffer m_buffer;
    };

  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_CRASH_HANDLER_FILTER_HPP
