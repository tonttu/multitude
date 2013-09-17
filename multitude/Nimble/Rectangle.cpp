/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Rectangle.hpp"

#include <numeric>

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

  Rectangle::Rectangle(Nimble::SizeF size, const Nimble::Matrix3f & m)
  {
    // Transform the points
    m_origin = m.project(Nimble::Vector2f(0, 0));
    Nimble::Vector2f c0 = m.project(Vector2f(0.5f * size.width(), 0.f));
    Nimble::Vector2f c1 = m.project(Vector2f(0.f, 0.5f * size.height()));

    // Compute the axes and extents
    m_axis0 = c0 - m_origin;
    m_axis1 = c1 - m_origin;

    m_extent0 = m_axis0.length();
    m_extent1 = m_axis1.length();

    m_axis0.normalize();
    m_axis1.normalize();
  }

  Rectangle::Rectangle(const Nimble::Rectf & rect)
  {
    m_origin = rect.center();
    m_axis0 = Nimble::Vector2f(1, 0);
    m_axis1 = Nimble::Vector2f(0, 1);
    m_extent0 = rect.width() * 0.5f;
    m_extent1 = rect.height() * 0.5f;
  }

  bool Rectangle::isInside(Nimble::Vector2f p) const
  {
    p -= m_origin;

    float u = std::abs(dot(p, m_axis0));
    float v = std::abs(dot(p, m_axis1));

    return (0 <= u && u <= m_extent0) && (0 <= v && v <= m_extent1);
  }

  bool Rectangle::isInside(const Nimble::Rectangle & r) const
  {
    // rectangle is inside if all points are inside
    std::array<Nimble::Vector2f, 4> corners;
    r.computeCorners(corners);

    for(int i = 0; i < 4; ++i)
      if(!isInside(corners[i])) return false;
    return true;
  }

  namespace
  {
    float cross(Nimble::Vector2f v, Nimble::Vector2f w)
    {
      return v.x*w.y - v.y*w.x;
    }
  }

  bool Rectangle::intersects(const Rectangle & r) const
  {
    // Check if other center is inside another rectangle
    if(r.isInside(m_origin) || isInside(r.m_origin))
        return true;

    // Fast negation in clear cases
    Nimble::Vector2f d = r.m_origin - m_origin;
    if(d.length() > Nimble::Math::Max(m_extent0, m_extent1) + Nimble::Math::Max(r.m_extent0, r.m_extent1))
        return false;

    // Do brute force checking of line segments
    std::array<Nimble::Vector2f, 4> corners;
    std::array<Nimble::Vector2f, 4> thatCorners;
    computeCorners(corners);
    r.computeCorners(thatCorners);

    for(int i = 0; i < 4; ++i) {
      int nextI = (i+1) % 4;
      for(int j = 0; j < 4; ++j) {
        int nextJ = (j+1) % 4;

        Nimble::Vector2f diff = corners[nextI] - corners[i];
        Nimble::Vector2f thatDiff = thatCorners[nextJ] - thatCorners[j];

        float a = cross(diff, thatDiff);
        if(a == 0.f) continue; // lines are parallel, just ignore

        Nimble::Vector2f startDiff = corners[j] - corners[i];
        float u = cross(startDiff, diff) / a;
        float t = cross(startDiff, thatDiff) / a;

        if(0 <= u && u <= 1 && 0 <= t && t <= 1)
          return true;
      }
    }
    return false;
  }

  Nimble::SizeF Rectangle::size() const
  {
    return Nimble::SizeF(2 * m_extent0, 2 * m_extent1);
  }

  void Rectangle::computeCorners(std::array<Nimble::Vector2f, 4> & corners) const
  {
    Nimble::Vector2f extAxis0 = m_axis0 * m_extent0;
    Nimble::Vector2f extAxis1 = m_axis1 * m_extent1;

    corners[0] = m_origin - extAxis0 - extAxis1;
    corners[1] = m_origin + extAxis0 - extAxis1;
    corners[2] = m_origin + extAxis0 + extAxis1;
    corners[3] = m_origin - extAxis0 + extAxis1;
  }

  Nimble::Rectangle Nimble::Rectangle::merge(const Nimble::Rectangle &a, const Nimble::Rectangle &b)
  {
    Rectangle box;

    // Average the centers
    box.m_origin = 0.5f * (a.center() + b.center());

    // Average the box axes (and negate if necessary)
    if(dot(a.m_axis0, b.m_axis0) >= 0.f) {
      box.m_axis0 = 0.5f * (a.m_axis0 + b.m_axis0);
      box.m_axis0.normalize();
    } else {
      box.m_axis0 = 0.5f * (a.m_axis0 - b.m_axis0);
      box.m_axis0.normalize();
    }

    box.m_axis1 = -box.m_axis0.perpendicular();
    box.m_axis1.normalize();

    // Project input corner points on the new axii and compute the extents
    std::array<Nimble::Vector2f, 4> vertex;
    Nimble::Vector2f min(0, 0);
    Nimble::Vector2f max(0, 0);
    const Nimble::Vector2f axes[] =  { box.m_axis0, box.m_axis1 };

    a.computeCorners(vertex);

    for(int i = 0; i < 4; i++) {

      Nimble::Vector2f diff = vertex[i] - box.center();

      for(int j = 0; j < 2; j++) {

        float dotp = dot(diff, axes[j]);

        if(dotp > max[j])
          max[j] = dotp;
        else if(dotp < min[j])
          min[j] = dotp;
      }
    }

    b.computeCorners(vertex);

    for(int i = 0; i < 4; i++) {

      Nimble::Vector2f diff = vertex[i] - box.center();

      for(int j = 0; j < 2; j++) {

        float dotp = dot(diff, axes[j]);

        if(dotp > max[j])
          max[j] = dotp;
        else if(dotp < min[j])
          min[j] = dotp;
      }
    }

    // Adjust the center and extents so the origin is at the center of the box
    box.m_origin += box.m_axis0 * 0.5f * (max[0] + min[0]);
    box.m_extent0 = 0.5f * (max[0] - min[0]);

    box.m_origin += box.m_axis1 * 0.5f * (max[1] + min[1]);
    box.m_extent1 = 0.5f * (max[1] - min[1]);

    return box;
  }

  void Rectangle::transform(const Nimble::Matrix3 &m)
  {
    std::array<Nimble::Vector2, 4> vertex;
    computeCorners(vertex);

    for(size_t i = 0; i < 4; i++)
      vertex[i] = m.project(vertex[i]);

    m_origin = 0.25f * std::accumulate(vertex.begin(), vertex.end(), Nimble::Vector2(0, 0));
    m_axis0 = vertex[1] - vertex[0];
    m_extent0 = 0.5f * m_axis0.length();
    m_axis0.normalize();

    m_axis1 = vertex[3] - vertex[0];
    m_extent1 = 0.5f * m_axis1.length();
    m_axis1.normalize();
  }

  Nimble::Rect Rectangle::boundingBox() const
  {
    const Nimble::Vector2f extAxis0 = m_axis0 * m_extent0;
    const Nimble::Vector2f extAxis1 = m_axis1 * m_extent1;

    Nimble::Rectf bb;
    bb.expand(m_origin - extAxis0 - extAxis1);
    bb.expand(m_origin + extAxis0 - extAxis1);
    bb.expand(m_origin + extAxis0 + extAxis1);
    bb.expand(m_origin - extAxis0 + extAxis1);

    return bb;
  }

}
