#ifndef TOUCHEVENT_HPP
#define TOUCHEVENT_HPP

#include "Export.hpp"

#include <QTouchEvent>

namespace Radiant
{
  /** Window system touch event. These are not Cornerstone touch events,
      but can be converted to such. */
  class RADIANT_API TouchEvent
  {
  public:

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

    const TouchPointList & touchPoints() const;
    TouchPointList & touchPoints();

    Type type() const;

  private:
    class D;
    D * m_d;
  };

}

#endif // TOUCHEVENT_HPP
