#ifndef TOUCHEVENT_HPP
#define TOUCHEVENT_HPP

#include "Export.hpp"

#include <Nimble/Vector2.hpp>

#include <memory>

namespace Radiant
{
  /** Window system touch event. These are not Cornerstone touch events,
      but can be converted to such.

  @ref MultiWidgets::Application listens to these events and uses them.
  */
  class TouchEvent
  {
  public:
    /// Possible touch event types
    enum Type {
      TOUCH_BEGIN, ///< First contact of the touch
      TOUCH_UPDATE, ///< Update to already detected touch
      TOUCH_END ///< End of touch
    };

    enum Flag
    {
      /// No flags
      FLAG_NONE       = 0,

      /// Event originates from a RawInputHandler. Event raw location uses
      /// a relative units and has value of 0..1 instead of himetric units
      FLAG_RAW        = 1 << 7,
    };
    typedef Radiant::FlagsT<Flag> Flags;

    /// Construct a new TouchEvent
    TouchEvent(int id = -1, Type type = TOUCH_BEGIN, Nimble::Vector2f location = {0, 0})
      : m_id(id)
      , m_type(type)
      , m_location(location)
    {}

    /// Copy constructor
    /// @param te TouchEvent to copy
    TouchEvent(const TouchEvent & te) = default;

    /// Touch event id
    int id() const { return m_id; }

    /// Returns the event type.
    /// @return Type of the event.
    Type type() const { return m_type; }

    /// Event flags
    Flags flags() const { return m_flags; }
    void setFlags(Flags flags) { m_flags = flags; }

    /// Touch point location in desktop coordinates
    Nimble::Vector2f location() const { return m_location; }
    void setLocation(Nimble::Vector2f location) { m_location = location; }

    /// Raw event location in device coordinates. In windows this is in
    /// himetric units or relative units from 0..1 depending on FLAG_RAW
    Nimble::Vector2f rawLocation() const { return m_rawLocation; }
    void setRawLocation(Nimble::Vector2f location) { m_rawLocation = location; }

    /// Unique ID for the source device. This can be typecasted to device
    /// HANDLE in Windows.
    uint64_t sourceDevice() const { return m_sourceDevice; }
    void setSourceDevice(uint64_t device) { m_sourceDevice = device; }

    /// Event time in seconds from arbitrary base time. In Windows this is
    /// performance counter value converted to seconds.
    double time() const { return m_time; }
    void setTime(double t) { m_time = t; }

  private:
    int m_id;
    Type m_type;
    Flags m_flags = FLAG_NONE;
    Nimble::Vector2f m_location;
    Nimble::Vector2f m_rawLocation{-1, -1};
    uint64_t m_sourceDevice = 0;
    double m_time = 0;
  };
  MULTI_FLAGS(TouchEvent::Flag)

}

#endif // TOUCHEVENT_HPP
