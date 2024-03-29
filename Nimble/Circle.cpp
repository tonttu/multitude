/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
