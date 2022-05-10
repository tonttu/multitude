/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_SPLINES_HPP
#define NIMBLE_SPLINES_HPP

#include "Export.hpp"
#include "Vector2.hpp"

#include <cstddef>
#include <vector>

namespace Luminous {
  class RenderContext;
  class Spline;
}

namespace Nimble
{

  /// Evaluate a cubic Catmull-Rom spline on an interval.
  /// @param t interpolation parameter [0, 1]
  /// @param cp vector of control points (needs at least four)
  /// @param index to the first control point used in interpolation. Used
  ///        control points will be from [index, index + 3]
  /// @return value of the spline at given t
  template<class T>
  T evalCatmullRom(float t, const std::vector<T> & cp, size_t index = 0)
  {
    // We need at least four control points to interpolate
    if(cp.size() < 4) {
      T zero;
      memset(&zero, 0, sizeof(zero));
      return zero;
    }

    float b1 = 0.5 * (-    t * t * t + 2 * t * t - t);
    float b2 = 0.5 * ( 3 * t * t * t - 5 * t * t + 2);
    float b3 = 0.5 * (-3 * t * t * t + 4 * t * t + t);
    float b4 = 0.5 * (     t * t * t -     t * t);

    size_t i1 = index + 0;
    size_t i2 = index + 1;
    size_t i3 = index + 2;
    size_t i4 = index + 3;

    return b1 * cp[i1] + b2 * cp[i2] + b3 * cp[i3] + b4 * cp[i4];
  }

  /// Evaluate the derivative of a Catmull-Rom spline at the given position
  template<class T>
  T evalCatmullRomDerivate(float t, const std::vector<T> & cp, size_t index)
  {
    // We need at least four control points to interpolate
    if(cp.size() < 4) {
      T zero;
      memset(&zero, 0, sizeof(zero));
      return zero;
    }

    const float b1 = -1.5f * t * t + 2.f * t - 0.5f;
    const float b2 = t * (4.5f * t - 5.0f);
    const float b3 = -4.5f * t * t + 4.0f * t + 0.5f;
    const float b4 = t * (1.5f * t - 1.0f);

    size_t i1 = index + 0;
    size_t i2 = index + 1;
    size_t i3 = index + 2;
    size_t i4 = index + 3;

    return b1 * cp[i1] + b2 * cp[i2] + b3 * cp[i3] + b4 * cp[i4];
  }

  /// Catmull-Rom
  /// @todo doc
  class NIMBLE_API Interpolating {
  public:
    /// Get derivative at the given interpolation point
    Nimble::Vector2 getDerivative(size_t ii, float t) const;
    /// Evaluates the spline at given t
    /// @param t position where to evaluate the spline. 0 <= t <= size() - 1
    /// @return Interpolated point on spline
    Nimble::Vector2 get(float t) const;
    /// Adds a control point
    void add(Nimble::Vector2 point);
    /// Removes the control point at the given index
    void remove(size_t ii);
    /// Returns the number of control points
    size_t size() const { return m_points.size(); }
    /// Returns the ith control point
    Nimble::Vector2 getControlPoint(size_t i) const { return m_points[i]; }

    /// Clears the interpolation key-points
    void clear();

    friend class Luminous::RenderContext;
    friend class Luminous::Spline;
  private:
    typedef std::vector<Nimble::Vector2> PointList;

    PointList m_points;
    PointList m_tangents;

    /// @todo why are these private?
    Nimble::Vector2 get(size_t ii, float h1, float h2, float h3, float h4) const;
    Nimble::Vector2 getPoint(size_t ii, float t) const;
  };

}

#endif
