/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include "Event.hpp"

namespace Valuable
{
#ifdef RADIANT_WINDOWS
  enum class PowerEvent
  {
    // This is raised immediately after we know we are entering suspend mode.
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
