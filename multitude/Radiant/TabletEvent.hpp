/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#ifndef RADIANT_TABLETEVENT_HPP
#define RADIANT_TABLETEVENT_HPP

#include <Radiant/Export.hpp>
#include <Nimble/Vector2.hpp>
#include <QTabletEvent>

namespace Radiant
{
  class TabletEvent
  {
  public:
    RADIANT_API TabletEvent();
    RADIANT_API TabletEvent(QTabletEvent & event);

    RADIANT_API void setLocation(const Nimble::Vector2f & location);
    RADIANT_API const Nimble::Vector2f & location() const;

    RADIANT_API QEvent::Type type() const;
  private:
    Nimble::Vector2f m_location;
    float pressure;
    QEvent::Type m_type;
  };

}

#endif
