/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "SystemEvents.hpp"

#ifdef RADIANT_WINDOWS
#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include <Windows.h>
#include <powrprof.h>
#endif

namespace Valuable
{
#ifdef RADIANT_WINDOWS
  namespace
  {
    Event<PowerEvent> s_onPowerChange;
    bool s_onPowerChangeInitialized = false;
    QMutex s_onPowerChangeInitMutex;

    ULONG powerChangeCallback(PVOID /* context */, ULONG type, PVOID /* setting */)
    {
      if (type == PBT_APMRESUMEAUTOMATIC) {
        s_onPowerChange.raise(Valuable::PowerEvent::RESUME);
      } else if (type == PBT_APMSUSPEND) {
        s_onPowerChange.raise(Valuable::PowerEvent::SUSPEND1);
        s_onPowerChange.raise(Valuable::PowerEvent::SUSPEND2);
      }

      return 0;
    }
  }

  Event<PowerEvent> & onPowerChange()
  {
    QMutexLocker g(&s_onPowerChangeInitMutex);
    if (s_onPowerChangeInitialized)
      return s_onPowerChange;

    DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS params;
    params.Callback = powerChangeCallback;
    params.Context = nullptr;
    if (!RegisterSuspendResumeNotification(&params, DEVICE_NOTIFY_CALLBACK)) {
      Radiant::error("RegisterSuspendResumeNotification failed: %s",
                     Radiant::StringUtils::getLastErrorMessage().toUtf8().data());
    }

    s_onPowerChangeInitialized = true;

    return s_onPowerChange;
  }
#endif
}
