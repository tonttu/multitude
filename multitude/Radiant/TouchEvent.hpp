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

    /// Raw event location in device coordinates. In windows this is in himetric units
    Nimble::Vector2f rawLocation() const { return m_rawLocation; }
    void setRawLocation(Nimble::Vector2f location) { m_rawLocation = location; }

    /// Unique ID for the source device. This can be typecasted to device
    /// HWND in Windows.
    uint64_t sourceDevice() const { return m_sourceDevice; }
    void setSourceDevice(uint64_t device) { m_sourceDevice = device; }

  private:
    int m_id;
    Type m_type;
    Nimble::Vector2f m_location;
    Nimble::Vector2f m_rawLocation{-1, -1};
    uint64_t m_sourceDevice = 0;
  };

}

#endif // TOUCHEVENT_HPP
