/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef RADIANT_PEN_EVENT_HPP
#define RADIANT_PEN_EVENT_HPP

#include "Flags.hpp"

#include <Nimble/Vector2.hpp>

class QTabletEvent;

namespace Radiant
{
  /// This class describes a tablet or pen event.
  class PenEvent
  {
  public:
    enum Flag
    {
      /// No flags
      FLAG_NONE       = 0,
      /// Event pressure is defined
      FLAG_PRESSURE   = 1 << 0,
      /// Event rotation is defined
      FLAG_ROTATION   = 1 << 1,
      /// Event tilt x is defined
      FLAG_TILT_X     = 1 << 2,
      /// Event tilt y is defined
      FLAG_TILT_Y     = 1 << 3,

      /// The barrel button is pressed
      FLAG_BARREL     = 1 << 4,
      /// The pen is inverted
      FLAG_INVERTED   = 1 << 5,
      /// The eraser button is pressed
      FLAG_ERASER     = 1 << 6,
    };
    typedef Radiant::FlagsT<Flag> Flags;

    enum Type
    {
      TYPE_NONE,
      TYPE_DOWN,
      TYPE_UP,
      TYPE_UPDATE,
    };

  public:
    /// Default constructor, members are initilized by default values
    PenEvent() = default;

    /// Initializes event based on corresponding
    /// @ref http://doc.qt.io/qt-5/qtabletevent.html "QTabletEvent".
    RADIANT_API PenEvent(QTabletEvent & event);

    uint32_t id() const { return m_id; }
    void setId(uint32_t id) { m_id = id; }

    /// Get location of the event
    /// @return Location os event
    const Nimble::Vector2f & location() const { return m_location; }
    /// Sets event's location
    /// @param location New location to set
    void setLocation(Nimble::Vector2f location) { m_location = location; }

    Type type() const { return m_type; }
    void setType(Type type) { m_type = type; }

    Flags flags() const { return m_flags; }
    void setFlags(Flags flags) { m_flags = flags; }

    float pressure() const { return m_pressure; }
    void setPressure(float pressure) { m_pressure = pressure; }

    float rotation() const { return m_rotation; }
    void setRotation(float rotation) { m_rotation = rotation; }

    // X and Y tilting angle between -pi/2 to pi/2
    Nimble::Vector2f tilt() const { return m_tilt; }
    void setTilt(Nimble::Vector2f tilt) { m_tilt = tilt; }

  private:
    Nimble::Vector2f m_location = {0, 0};
    Type m_type = TYPE_NONE;
    Flags m_flags = FLAG_NONE;
    uint32_t m_id = 0;
    float m_pressure = 0;
    float m_rotation = 0;
    // X and Y tilting between -pi/2 to pi/2
    Nimble::Vector2f m_tilt;
  };

}

#endif
