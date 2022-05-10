#ifndef TOUCHEVENT_HPP
#define TOUCHEVENT_HPP

#include "PenEvent.hpp"

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

    using RawLocationType = PenEvent::RawLocationType;

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

    /// Touch point location in desktop coordinates
    Nimble::Vector2f location() const { return m_location; }
    void setLocation(Nimble::Vector2f location) { m_location = location; }

    /// Raw event location in device coordinates. Only valid if FLAG_RAW_LOCATION
    /// is set. Notice that this is in different units than location and
    /// also different from PenData::rawLocation.
    /// @sa rawLocationType
    Nimble::Vector2f rawLocation() const { return m_rawLocation; }
    void setRawLocation(Nimble::Vector2f location) { m_rawLocation = location; }

    /// How to interpret rawLocation values.
    RawLocationType rawLocationType() const { return m_rawLocationType; }
    void setRawLocationType(RawLocationType rawLocationType) { m_rawLocationType = rawLocationType; }

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
    RawLocationType m_rawLocationType = PenEvent::RAW_LOCATION_HIMETRIC;
    Nimble::Vector2f m_location;
    Nimble::Vector2f m_rawLocation{-1, -1};
    uint64_t m_sourceDevice = 0;
    double m_time = 0;
  };

}

#endif // TOUCHEVENT_HPP
