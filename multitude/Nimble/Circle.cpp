#include "Circle.hpp"

namespace Nimble {

  Circle::Circle()
    : m_center(Nimble::Vector2f(0.f,0.f))
    , m_radius(1.f)
    , m_radiusSquared(1.f)
  {}

  Circle::Circle(const Nimble::Vector2f & center, float radius)
    : m_center(center)
    , m_radius(radius)
    , m_radiusSquared(radius*radius)
  {
  }

  Circle::Circle(const Nimble::Circle & circle)
  {
    m_center = circle.center();
    m_radius = circle.radius();
    m_radiusSquared = m_radius*m_radius;
  }

  Nimble::Rectf Circle::boundingBox() const {
    return Nimble::Rectf(m_center.x-m_radius,m_center.y-m_radius, m_center.x+m_radius,m_center.y+m_radius);
  }

  bool Circle::contains(const Nimble::Rectf & rect) const
  {
    bool result{true};

    // If any corners is > m_radius from m_centre, the
    // rectangle is not contained

    auto corners = rect.computeCorners();
    for (auto c : corners) {
      auto delta = c-m_center;
      if (delta.lengthSqr() > m_radiusSquared) {
        result=false;
        break;
      }
    }

    return result;
  }

  bool Circle::contains(const Nimble::Vector2f & point) const
  {
    auto delta = point-m_center;
    return delta.lengthSqr() <= m_radiusSquared;
  }


  bool Circle::intersects(const Nimble::Rectf & rect) const
  {
    return (rect.clamp(m_center) - m_center).lengthSqr() <= m_radiusSquared;
  }

}
