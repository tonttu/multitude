#include "TraceDuplicateFilter.hpp"

namespace Radiant
{
  namespace Trace
  {
    DuplicateFilter::DuplicateFilter()
      : Filter(ORDER_DEFAULT_FILTERS + 1.0f)
    {}

    bool DuplicateFilter::trace(const Message & message)
    {
      Guard g(m_msgMutex);
      if (m_prevMessage == message) {
        return true;
      }

      m_prevMessage = message;
      return false;
    }

  } // namespace Trace
} // namespace Radiant
