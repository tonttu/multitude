#ifndef TOUCHEVENT_HPP
#define TOUCHEVENT_HPP

#include "Export.hpp"

#include <QTouchEvent>

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

    /// List of touchpoints
    typedef QList<QTouchEvent::TouchPoint> TouchPointList;

    /// Constructor. Type of the event is set as @ref TOUCH_BEGIN
    TouchEvent();
    /// Copy constructor
    /// @param te TouchEvent to copy
    TouchEvent(const TouchEvent &te);
    /// Construct TouchEvent from QTouchEvent
    /// @param qte TouchEvent to duplicate
    TouchEvent(const QTouchEvent &qte);

    /// Destructor
    virtual ~TouchEvent();

    /// Single event can contain multiple touch points attached to it.
    /// @return The list of touch points contained in the touch event.
    const TouchPointList & touchPoints() const;
    /// @copydoc touchPoints
    TouchPointList & touchPoints();
    /// Returns the event type.
    /// @return Type of the event.
    Type type() const;

  private:
    class D;
    D * m_d;
  };

}

#endif // TOUCHEVENT_HPP
