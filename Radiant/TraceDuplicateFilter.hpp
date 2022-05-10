#ifndef RADIANT_TRACE_DUPLICATE_FILTER_HPP
#define RADIANT_TRACE_DUPLICATE_FILTER_HPP

#include "Trace.hpp"
#include "Mutex.hpp"

namespace Radiant
{
  namespace Trace
  {
    /// Drops duplicate messages
    class RADIANT_API DuplicateFilter : public Filter
    {
    public:
      /// Creates a new filter using order ORDER_DEFAULT_FILTERS + 1, so that
      /// this filter is executed after the other default filters
      DuplicateFilter();

      bool trace(Message & message) override;

    private:
      Message m_prevMessage;
      Mutex m_msgMutex;
    };
  } // namespace Trace
} // namespace Radiant

#endif // RADIANT_TRACE_DUPLICATE_FILTER_HPP
