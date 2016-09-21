#include "Circle.hpp"

namespace Nimble {

  Circle::Circle(const Nimble::Vector2f & center, float radius)
    : m_center(center)
    , m_radius(radius)
    , m_radiusSquared(radius*radius)
  {
  }

  Nimble::Rect Circle::boundingBox() const {
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
      float distanceSquared = std::pow(delta.x,2) + std::pow(delta.y,2);
      if (distanceSquared > m_radiusSquared) {
        result=false;
        break;
      }
    }

    return result;
  }

  bool Circle::contains(const Nimble::Vector2f & point) const
  {
    auto delta = point-m_center;
    float distanceSquared = std::pow(delta.x,2) + std::pow(delta.y,2);
    return distanceSquared <= m_radiusSquared;
  }


  bool Circle::intersects(const Nimble::Rectf & rect) const
  {
    Nimble::Vector2f rectCenter = rect.center();
    Nimble::Vector2f circleDistance( fabs(m_center.x - rectCenter.x), fabs(m_center.y - rectCenter.y));

    // Detect easy case where circle bounds are outside rect bounds
    if (circleDistance.x > (rect.width()/2 + m_radius)) { return false; }
    if (circleDistance.y > (rect.height()/2 + m_radius)) { return false; }

    // Also easy if the circle center is inside the rect
    if (circleDistance.x <= (rect.width()/2)) { return true; }
    if (circleDistance.y <= (rect.height()/2)) { return true; }

    // Check the edge case where the circle is near the corner
    float cornerDistanceSquared = std::pow(circleDistance.x - rect.width()/2,2) +
                                  std::pow(circleDistance.y - rect.height()/2,2);
    return (cornerDistanceSquared <= (m_radiusSquared));
  }

}
