#include "Event.hpp"

#ifdef RADIANT_WINDOWS
namespace
{
  thread_local uint32_t t_removeCurrentEventListener{0};
}

namespace Valuable
{
  uint32_t & removeCurrentEventListenerCounter()
  {
    return t_removeCurrentEventListener;
  }
}
#else
namespace Valuable
{
  MULTI_DLLEXPORT thread_local uint32_t t_removeCurrentEventListenerCounter{0};
}

#endif
