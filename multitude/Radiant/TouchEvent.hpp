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


    typedef QList<QTouchEvent::TouchPoint> TouchPointList;

    TouchEvent();
    TouchEvent(const TouchEvent &);
    TouchEvent(const QTouchEvent &);

    virtual ~TouchEvent();

    const TouchPointList & touchPoints() const;
    TouchPointList & touchPoints();

  private:
    class D;
    D * m_d;
  };

}

#endif // TOUCHEVENT_HPP
