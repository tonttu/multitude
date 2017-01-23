/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Radiant/PenEvent.hpp"

#include <QTabletEvent>

namespace Radiant
{
  PenEvent::PenEvent(QTabletEvent & event)
  {
    m_location.make(event.hiResGlobalX(), event.hiResGlobalY());
    if (event.type() == QEvent::TabletPress) {
      m_type = TYPE_DOWN;
    } else if (event.type() == QEvent::TabletMove) {
      m_type = TYPE_UPDATE;
    } else if (event.type() == QEvent::TabletRelease) {
      m_type = TYPE_UP;
    }
    m_pressure = event.pressure();
  }
}
