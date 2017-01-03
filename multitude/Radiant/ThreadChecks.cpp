#include "ThreadChecks.hpp"

namespace Radiant
{
  namespace ThreadInfo
  {
    Thread::id_t mainThreadId = Thread::myThreadId();
  }
}
