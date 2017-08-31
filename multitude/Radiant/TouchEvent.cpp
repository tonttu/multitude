#include "TouchEvent.hpp"


#include "Trace.hpp"

namespace Radiant
{

  class TouchEvent::D
  {
  public:
    D() : m_type(TOUCH_BEGIN) {}

    Type m_type;

    QList<QTouchEvent::TouchPoint> m_points;
  };

  TouchEvent::TouchEvent()
    : m_d(new D())
  {}

  TouchEvent::TouchEvent(const TouchEvent & e)
    : m_d(new D())
  {
    *m_d = *e.m_d;
  }

  TouchEvent::TouchEvent(TouchEvent && e)
    : m_d(std::move(e.m_d))
  {
  }

  TouchEvent::TouchEvent(const QTouchEvent & e)
    : m_d(new D())
  {
    m_d->m_points = e.touchPoints();

    if(e.type() == QEvent::TouchBegin)
      m_d->m_type = TOUCH_BEGIN;
    else if(e.type() == QEvent::TouchUpdate)
      m_d->m_type = TOUCH_UPDATE;
    else if(e.type() == QEvent::TouchEnd)
      m_d->m_type = TOUCH_END;
    else
      Radiant::error("TouchEvent::TouchEvent(const QTouchEvent & e) # Unknown type");
  }

  TouchEvent::TouchEvent(TouchEvent::Type type, const TouchEvent::TouchPointList & points)
    : m_d(new D())
  {
    m_d->m_type = type;
    m_d->m_points = points;
  }

  TouchEvent::~TouchEvent()
  {
  }

  const TouchEvent::TouchPointList &TouchEvent::touchPoints() const
  {
    return m_d->m_points;
  }

  TouchEvent::TouchPointList &TouchEvent::touchPoints()
  {
    return m_d->m_points;
  }

  Radiant::TouchEvent::Type Radiant::TouchEvent::type() const
  {
    return m_d->m_type;
  }

}
