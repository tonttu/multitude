#include "Rectangle.hpp"

namespace Nimble
{
  Rectangle::Rectangle()
  {}

  Rectangle::Rectangle(Nimble::Vector2f origin, Nimble::Vector2f a0, float e0, Nimble::Vector2f a1, float e1)
      : m_origin(origin),
      m_axis0(a0),
      m_axis1(a1),
      m_extent0(e0),
      m_extent1(e1)
  {}

  Rectangle::Rectangle(Nimble::Vector2f size, const Nimble::Matrix3f & m)
  {
    // Transform the points
    m_origin = (m * Vector2f(0, 0)).xy();
    Vector2f c0 = (m * Vector2f(0.5f * size.x, 0.f)).xy();
    Vector2f c1 = (m * Vector2f(0.f, 0.5f * size.y)).xy();

    // Compute the axii and extents
    m_axis0 = c0 - m_origin;
    m_axis1 = c1 - m_origin;

    m_extent0 = m_axis0.length();
    m_extent1 = m_axis1.length();

    m_axis0.normalize();
    m_axis1.normalize();
  }
  
  bool Rectangle::inside(Nimble::Vector2f p) const
  {
    p -= m_origin;

    float u = Math::Abs(dot(p, m_axis0));
    float v = Math::Abs(dot(p, m_axis1));

    return (0 <= u && u <= m_extent0) && (0 <= v && v <= m_extent1);
  }

  bool Rectangle::intersects(const Rectangle & r) const
  {
    // Difference box centers
    Vector2f d = r.m_origin - m_origin;

    float absAdB[2][2];

    absAdB[0][0] = Math::Abs(dot(m_axis0, r.m_axis0));
    absAdB[0][1] = Math::Abs(dot(m_axis0, r.m_axis1));
    float absAdD = Math::Abs(dot(m_axis0, d));
    float sum = m_extent0 + r.m_extent0 * absAdB[0][0] + r.m_extent1 * absAdB[0][1];
    if(absAdD > sum)
      return false;

    absAdB[1][0] = Math::Abs(dot(m_axis1, r.m_axis0));
    absAdB[1][1] = Math::Abs(dot(m_axis1, r.m_axis1));
    absAdD = Math::Abs(dot(m_axis1, d));
    sum = m_extent1 + r.m_extent0 * absAdB[1][0] + r.m_extent1 * absAdB[1][1];
    if(absAdD > sum)
      return false;

    absAdD = Math::Abs(dot(r.m_axis0, d));
    sum = r.m_extent0 + m_extent0 * absAdB[0][0] + m_extent1 * absAdB[1][0];
    if(absAdD > sum)
      return false;

    absAdD = Math::Abs(dot(r.m_axis1, d));
    sum = r.m_extent1 + m_extent0 * absAdB[0][1] + m_extent1 * absAdB[1][1];
    if(absAdD > sum)
      return false;  

    return true;
  }

  Nimble::Vector2f Rectangle::center() const
  {
    return m_origin;
  }

  Nimble::Vector2 Rectangle::size() const
  {
    return Nimble::Vector2(2 * m_extent0, 2 * m_extent1);
  }
}
