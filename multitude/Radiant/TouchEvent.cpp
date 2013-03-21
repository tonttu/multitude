#include "TouchEvent.hpp"

namespace Radiant
{

  class TouchEvent::D
  {
  public:

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

  TouchEvent::TouchEvent(const QTouchEvent & e)
    : m_d(new D())
  {
    m_d->m_points = e.touchPoints();
  }

  TouchEvent::~TouchEvent()
  {
    delete m_d;
  }

  const TouchEvent::TouchPointList &TouchEvent::touchPoints() const
  {
    return m_d->m_points;
  }

  TouchEvent::TouchPointList &TouchEvent::touchPoints()
  {
    return m_d->m_points;
  }

}
