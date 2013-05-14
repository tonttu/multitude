/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef RADIANT_TABLETEVENT_HPP
#define RADIANT_TABLETEVENT_HPP

#include <Radiant/Export.hpp>
#include <Nimble/Vector2.hpp>
#include <QTabletEvent>

namespace Radiant
{
  /// This class describes a (Wacom) tablet event. Tablet events behave
  /// similar to mouse events. They are created when the application has input
  /// focus and a stylus is pressed, released or moved on a tablet.
  class TabletEvent
  {
  public:
    /// Default constructor, members are initilized by default values
    RADIANT_API TabletEvent();
    /// Initializes event based on corresponding
    /// @ref http://qt-project.org/doc/qt-4.8/qtabletevent.html "QTabletEvent".
    RADIANT_API TabletEvent(QTabletEvent & event);

    /// Sets event's location
    /// @param location New location to set
    RADIANT_API void setLocation(Nimble::Vector2f location);
    /// Get location of the event
    /// @return Location os event
    RADIANT_API const Nimble::Vector2f & location() const;

    /// Type of the event (TabletMove, TabletPress, TabletRelease)
    /// @return Type of the event
    RADIANT_API QEvent::Type type() const;

  private:
    Nimble::Vector2f m_location;
    QEvent::Type m_type;
  };

}

#endif
