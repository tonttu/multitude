/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include <Nimble/Path.hpp>
#include <Nimble/Random.hpp>

#include <cmath>
#include <numeric>

namespace {

  struct Segment {
    Nimble::Vector2f p0;
    Nimble::Vector2f p1;

    bool intersects(const Segment & seg) const
    {
      Nimble::Vector2f diff = seg.p0 - p0;
      
      Nimble::Vector2f d0 = (p1 - p0);
      float LS = d0.length();
      d0.normalize();

      Nimble::Vector2f d1 = (seg.p1 - seg.p0);
      float LT = d1.length();
      d1.normalize();

      float dotPerpD0D1 = d0.x * d1.y - d0.y * d1.x;

      // Parallel?
      if(fabs(dotPerpD0D1) < 1e-5) return false;
      
      float invDotPerpD0D1 = 1.f / dotPerpD0D1;
      float dotPerpDiffD0 = diff.x * d0.y - diff.y * d0.x;
      float dotPerpDiffD1 = diff.x * d1.y - diff.y * d1.x;

      float s = dotPerpDiffD1 * invDotPerpD0D1;
      if(s < 0 || s > LS) return false;

      float t = dotPerpDiffD0 * invDotPerpD0D1;
      if(t < 0 || t > LT) return false;

      return true;
    }

  };

  void simplifyDP(float tolerance, std::vector<Nimble::Vector2f> & points, int beg, int end, std::vector<bool> & markers)
  {
    if(end <= beg + 1) return;

    // Check for adequate approximation
    const float T2 = tolerance * tolerance;
    int maxIndex = beg;
    float maxDistSqr = 0;
    Segment segment = { points[beg], points[end] };
    Nimble::Vector2f u = segment.p1 - segment.p0;
    float cu = dot(u, u);

    // Find the point with the maximum distance to the segment
    for(int i = beg + 1; i < end; i++) {
      // Compute squared distance
      Nimble::Vector2f w = points[i] - segment.p0;
      float cw = dot(w, u);

      float dv2;
      if(cw <= 0) dv2 = (points[i] - segment.p0).lengthSqr();
      else if(cu <= cw) dv2 = (points[i] - segment.p1).lengthSqr();
      else {
        float s = cw / cu;
        Nimble::Vector2f perpBase = segment.p0 + s * u;
        dv2 = (points[i] - perpBase).lengthSqr();
      }

      if(dv2 <= maxDistSqr) continue;

      maxIndex = i;
      maxDistSqr = dv2;      
    }

    if(maxDistSqr > T2) {
      // Split the polyline at the farthest vertex
      markers[maxIndex] = true;
      simplifyDP(tolerance, points, beg, maxIndex, markers);
      simplifyDP(tolerance, points, maxIndex, end, markers);
    }
  }

}

namespace Nimble {

  Path::Path()
  {}

  void Path::simplify(float clusterTolerance, float dpTolerance)
  {
    //  Radiant::trace("Path::simplify # starting from %d points", m_points.size());
    const float to2 = clusterTolerance * clusterTolerance;

    container buffer;
    buffer.reserve(m_points.size());

    // Stage 1, remove adjacent vertices that are too close to each other
    buffer.push_back(m_points[0]);
    size_t pv = 0;
    for(size_t i = 1; i < m_points.size(); i++) {
      if( (m_points[i] - m_points[pv]).lengthSqr() < to2)
        continue;

      buffer.push_back(m_points[i]);
      pv = i;
    }
    // Make sure the last vertex is in
    if(pv < m_points.size() - 1)
      buffer.push_back(m_points.back());

    //  Radiant::trace("Path::simplify # after stage 1: %d points", buffer.size());

    // Stage 2, Douglas-Peucker simplification
    std::vector<bool> markers(m_points.size(), false);
    markers[0] = markers[buffer.size() - 1] = true;
    simplifyDP(dpTolerance, buffer, 0, (int) buffer.size() - 1, markers);

    // Copy the marked vertices to output
    m_points.clear();
    for(size_t i = 0; i < markers.size(); i++)
      if(markers[i])
        m_points.push_back(buffer[i]);

    //  Radiant::trace("Path::simplify # after stage 2: %d points", m_points.size());
  }

  void Path::transform(const Nimble::Matrix3f & m)
  {
    for(size_t i = 0; i < m_points.size(); i++) {
      Nimble::Vector3f p(m_points[i].x, m_points[i].y, 1);

      m_points[i] = (m * p).vector2();
    }
  }

  void Path::simplifyAngular(float degrees)
  {
    size_t i = 0;
    float cuma = 0.f;

    while(i < m_points.size() - 2) {
      Nimble::Vector2f p0 = m_points[i + 0];
      Nimble::Vector2f p1 = m_points[i + 1];
      Nimble::Vector2f p2 = m_points[i + 2];

      Nimble::Vector2f v0(p1 - p0);
      Nimble::Vector2f v1(p2 - p1);

      v0.normalize();
      v1.normalize();

      cuma += float(acos(dot(v0, v1)) * 180.0 / M_PI);

      if(cuma < degrees) {
        m_points.erase(m_points.begin() + i + 1);
      } else {
        cuma = fmodf(cuma, degrees);
        i++;
      }
    }
  }

  Nimble::Vector2f Path::center() const
  {
    Nimble::Vector2f c(0, 0);

    for(size_t i = 0; i < m_points.size(); i++) {
      c += m_points[i];
    }

    return c / static_cast<float>(m_points.size());
  }

  // Just brute force
  bool Path::intersect(const Path & p1, const Nimble::Matrix3f & m1, const Path & p2, const Nimble::Matrix3f & m2)
  {
    for(container::const_iterator i1 = p1.m_points.begin(); i1 != p1.m_points.end() - 1; ++i1) {

      Nimble::Vector3f v0(*i1, 1);
      Nimble::Vector3f v1(*(i1 + 1), 1);

      v0 = (m1 * v0);
      v1 = (m1 * v1);

      Segment s1 = { v0.vector2(), v1.vector2() };

      for(container::const_iterator i2 = p2.m_points.begin(); i2 != p2.m_points.end() - 1; ++i2) {

        Nimble::Vector3f w0(*i2, 1);
        Nimble::Vector3f w1(*(i2 + 1), 1);

        w0 = (m2 * w0);
        w1 = (m2 * w1);

        Segment s2 = { w0.vector2(), w1.vector2() };

        if(s1.intersects(s2))
          return true;
      }
    }

    return false;
  }

  bool Path::isDegenerate() const
  {
    if(m_points.size() == 2) {

      const Nimble::Vector2f v0 = m_points[0];
      const Nimble::Vector2f v1 = m_points[1];

      Nimble::Vector2f u(v1 - v0);
      if(u.lengthSqr() < 1e3)
        return true;
    }

    return false;
  }

}
