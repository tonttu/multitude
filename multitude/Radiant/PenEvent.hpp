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
      /// Raw location is defined
      FLAG_RAW_LOCATION = 1 << 4,

      /// The barrel button is pressed
      FLAG_BARREL     = 1 << 5,
      /// The pen is inverted
      FLAG_INVERTED   = 1 << 6,
      /// The eraser button is pressed
      FLAG_ERASER     = 1 << 7,
    };
    typedef Radiant::FlagsT<Flag> Flags;

    enum RawLocationType
    {
      /// Raw location is in himetric units
      RAW_LOCATION_HIMETRIC,
      /// Raw location is in relative units (0..1). This is used by RawInputHandler.
      RAW_LOCATION_RELATIVE,
    };

    enum Type
    {
      /// Invalid / null event
      TYPE_NONE,
      /// Pen was pressed
      TYPE_DOWN,
      /// Pen was released
      TYPE_UP,
      /// Pen state update
      TYPE_UPDATE,
    };

  public:
    /// Default constructor, members are initilized by default values
    inline PenEvent() = default;

    /// Initializes event based on corresponding
    /// @ref http://doc.qt.io/qt-5/qtabletevent.html "QTabletEvent".
    RADIANT_API PenEvent(QTabletEvent & event);

    /// Pen id. With some devices and some platforms the ids might not be
    /// unique and the ids could be reused.
    uint32_t id() const { return m_id; }
    void setId(uint32_t id) { m_id = id; }

    /// Get location of the event in desktop coordinate system
    /// @return Location os event
    const Nimble::Vector2f & location() const { return m_location; }
    /// Sets event's location
    /// @param location New location to set
    void setLocation(Nimble::Vector2f location) { m_location = location; }

    /// Event type
    Type type() const { return m_type; }
    void setType(Type type) { m_type = type; }

    /// Flags tell the state of possible buttons and whether pressure and
    /// other information is available
    Flags flags() const { return m_flags; }
    void setFlags(Flags flags) { m_flags = flags; }

    /// Pen tip pressure from 0 (pen is barely touching the screen) to 1
    /// (fully pressed). Only valid if FLAG_PRESSURE is set
    float pressure() const { return m_pressure; }
    void setPressure(float pressure) { m_pressure = pressure; }

    /// Pen rotation from 0 to 2pi. Only valid if FLAG_ROTATION is set
    float rotation() const { return m_rotation; }
    void setRotation(float rotation) { m_rotation = rotation; }

    /// X and Y tilting angle between -pi/2 to pi/2. (0, 0) means that the
    /// pen is perpendicular to the screen. Only valid if FLAG_TILT_* flags
    /// are set
    Nimble::Vector2f tilt() const { return m_tilt; }
    void setTilt(Nimble::Vector2f tilt) { m_tilt = tilt; }

    /// Raw event location in device coordinates. Only valid if FLAG_RAW_LOCATION
    /// is set. Notice that this is in different units than location and
    /// also different from PenData::rawLocation.
    /// @sa rawLocationType
    Nimble::Vector2f rawLocation() const { return m_rawLocation; }
    void setRawLocation(Nimble::Vector2f location) { m_rawLocation = location; }

    /// How to interpret rawLocation values.
    RawLocationType rawLocationType() const { return m_rawLocationType; }
    void setRawLocationType(RawLocationType rawLocationType) { m_rawLocationType = rawLocationType; }

    /// Unique ID for the source device. This can be used to differentiate
    /// between multiple pens, if the hardware supports that. This can be
    /// typecasted to device HANDLE in Windows.
    uint64_t sourceDevice() const { return m_sourceDevice; }
    void setSourceDevice(uint64_t device) { m_sourceDevice = device; }

    /// Event time in seconds from arbitrary base time. In Windows this is
    /// performance counter value converted to seconds.
    double time() const { return m_time; }
    void setTime(double t) { m_time = t; }

  private:
    Nimble::Vector2f m_location = {0, 0};
    Nimble::Vector2f m_rawLocation = {0, 0};
    Type m_type = TYPE_NONE;
    Flags m_flags = FLAG_NONE;
    RawLocationType m_rawLocationType = RAW_LOCATION_HIMETRIC;
    uint32_t m_id = 0;
    float m_pressure = 0;
    float m_rotation = 0;
    Nimble::Vector2f m_tilt = {0, 0};
    uint64_t m_sourceDevice = 0;
    double m_time = 0;
  };
  MULTI_FLAGS(PenEvent::Flag)
}

#endif
