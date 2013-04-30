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
      TOUCH_BEGIN,
      TOUCH_UPDATE,
      TOUCH_END
    };

    typedef QList<QTouchEvent::TouchPoint> TouchPointList;

    TouchEvent();
    TouchEvent(const TouchEvent &);
    TouchEvent(const QTouchEvent &);

    virtual ~TouchEvent();

    /** Single event can contain multiple touch points attached to it.
    Returns the list of touch points contained in the touch event.
    */
    const TouchPointList & touchPoints() const;
    /// @copydoc touchPoints
    TouchPointList & touchPoints();
    /// returns the event type.
    Type type() const;

  private:
    class D;
    D * m_d;
  };

}

#endif // TOUCHEVENT_HPP
