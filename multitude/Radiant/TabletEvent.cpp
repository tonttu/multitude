/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Radiant/TabletEvent.hpp"

namespace Radiant
{
  TabletEvent::TabletEvent()
  {
  }

  TabletEvent::TabletEvent(QTabletEvent & event)
  {
    m_location.make(event.hiResGlobalX(), event.hiResGlobalY());
    m_type = event.type();
  }

  QEvent::Type TabletEvent::type() const
  {
    return m_type;
  }

  void TabletEvent::setLocation(const Nimble::Vector2f & location)
  {
    m_location = location;
  }

  const Nimble::Vector2f & TabletEvent::location() const
  {
    return m_location;
  }
}
