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

  Rectangle::Rectangle(Nimble::Vector2f size, const Nimble::Matrix3f & m)
  {
    // Transform the points
    m_origin = m.project(Nimble::Vector2f(0, 0));
    Nimble::Vector2f c0 = m.project(Vector2f(0.5f * size.x, 0.f));
    Nimble::Vector2f c1 = m.project(Vector2f(0.f, 0.5f * size.y));

    // Compute the axii and extents
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
    std::vector<Nimble::Vector2f> corners;
    corners.reserve(4);
    r.computeCorners(corners);
    assert(corners.size() == 4);

    for(int i = 0; i < 4; ++i)
      if(!isInside(corners[i])) return false;
    return true;
  }

  bool Rectangle::intersects(const Rectangle & r) const
  {
    // Difference box centers
    Nimble::Vector2f d = r.m_origin - m_origin;

    float absAdB[2][2];

    absAdB[0][0] = std::abs(dot(m_axis0, r.m_axis0));
    absAdB[0][1] = std::abs(dot(m_axis0, r.m_axis1));
    float absAdD = std::abs(dot(m_axis0, d));
    float sum = m_extent0 + r.m_extent0 * absAdB[0][0] + r.m_extent1 * absAdB[0][1];
    if(absAdD > sum)
      return false;

    absAdB[1][0] = std::abs(dot(m_axis1, r.m_axis0));
    absAdB[1][1] = std::abs(dot(m_axis1, r.m_axis1));
    absAdD = std::abs(dot(m_axis1, d));
    sum = m_extent1 + r.m_extent0 * absAdB[1][0] + r.m_extent1 * absAdB[1][1];
    if(absAdD > sum)
      return false;

    absAdD = std::abs(dot(r.m_axis0, d));
    sum = r.m_extent0 + m_extent0 * absAdB[0][0] + m_extent1 * absAdB[1][0];
    if(absAdD > sum)
      return false;

    absAdD = std::abs(dot(r.m_axis1, d));
    sum = r.m_extent1 + m_extent0 * absAdB[0][1] + m_extent1 * absAdB[1][1];
    if(absAdD > sum)
      return false;

    return true;
  }

  Nimble::Vector2 Rectangle::size() const
  {
    return Nimble::Vector2(2 * m_extent0, 2 * m_extent1);
  }

  void Rectangle::computeCorners(std::vector<Nimble::Vector2f> &corners) const
  {
    Nimble::Vector2f extAxis0 = m_axis0 * m_extent0;
    Nimble::Vector2f extAxis1 = m_axis1 * m_extent1;

    corners.push_back(m_origin - extAxis0 - extAxis1);
    corners.push_back(m_origin + extAxis0 - extAxis1);
    corners.push_back(m_origin + extAxis0 + extAxis1);
    corners.push_back(m_origin - extAxis0 + extAxis1);
  }

  Nimble::Rectangle Nimble::Rectangle::merge(const Nimble::Rectangle &a, const Nimble::Rectangle &b)
  {
    Rectangle box;

    // Average the centers
    box.m_origin = 0.5f * (a.center() + b.center());

    // Average the box axii (and negate if necessary)
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
    std::vector<Nimble::Vector2f> vertex;
    Nimble::Vector2f min(0, 0);
    Nimble::Vector2f max(0, 0);
    const Nimble::Vector2f axii[] =  { box.m_axis0, box.m_axis1 };

    a.computeCorners(vertex);

    for(int i = 0; i < 4; i++) {

      Nimble::Vector2f diff = vertex[i] - box.center();

      for(int j = 0; j < 2; j++) {

        float dotp = dot(diff, axii[j]);

        if(dotp > max[j])
          max[j] = dotp;
        else if(dotp < min[j])
          min[j] = dotp;
      }
    }

    vertex.clear();
    b.computeCorners(vertex);

    for(int i = 0; i < 4; i++) {

      Nimble::Vector2f diff = vertex[i] - box.center();

      for(int j = 0; j < 2; j++) {

        float dotp = dot(diff, axii[j]);

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

//  void dumpRectangle(const Rectangle & r)
//  {
//    Radiant::info("Rectangle o(%f,%f) a0(%f,%f) e0(%f) a1(%f,%f) e1(%f)", r.center().x, r.center().y, r.axis0().x, r.axis0().y, r.extent0(), r.axis1().x, r.axis1().y, r.extent1());
//  }

  void Rectangle::transform(const Nimble::Matrix3 &m)
  {
    std::vector<Nimble::Vector2> vertex;
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
