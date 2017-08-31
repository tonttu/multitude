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
  class RADIANT_API TouchEvent
  {
  public:
    /// Possible touch event types
    enum Type {
      TOUCH_BEGIN, ///< First contact of the touch
      TOUCH_UPDATE, ///< Update to already detected touch
      TOUCH_END ///< End of touch
    };

    /// Construct a new TouchEvent
    TouchEvent(int id, Type type, Nimble::Vector2f location)
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

  private:
    int m_id;
    Type m_type;
    Nimble::Vector2f m_location;
  };

}

#endif // TOUCHEVENT_HPP
