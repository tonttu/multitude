#pragma once

#include "Event.hpp"

namespace Valuable
{
#ifdef RADIANT_WINDOWS
  enum class PowerEvent
  {
    // This is raised immediatey after we know we are entering suspend mode.
    // Do not block event handling, just start asynchronously preparing for
    // suspend.
    SUSPEND1,
    // Raised just after SUSPEND1. Event handlers should not return until
    // everything is ready for suspend.
    SUSPEND2,
    // Raised after resuming from suspend.
    RESUME,
  };

  VALUABLE_API Event<PowerEvent> & onPowerChange();
#endif
}
