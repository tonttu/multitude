/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_PATH_HPP
#define NIMBLE_PATH_HPP

#include "Export.hpp"

#include "Matrix3.hpp"
#include "Vector2.hpp"

#include <vector>

namespace Nimble {

  /// Path provides some utility functions to manipulate series of points  
  class NIMBLE_API Path
  {
    public:
      /// Container for points on path
      typedef std::vector<Nimble::Vector2f> container;

      Path();

      /// Adds a point to the end of the path
      void addPoint(Nimble::Vector2f p) { m_points.push_back(p); }

      /// Clears all points
      void clear() { m_points.clear(); }

      /// Simplifies the path using a two-pass algorithm. First, clustered
      /// points are merged together and after this a second pass simplification is
      /// performed using the Douglas-Plucker method.
      /// @param clusterTolerance points within this distance from eachother are merged together
      /// @param dpTolerance tolerance for Douglas-Plucker method
      void simplify(float clusterTolerance, float dpTolerance);

      /** Simplifies the path by removing points based on the angle each
      segment differs from the previous segment. Each segment that forms an
      angle less than the given tolerance from the previous segment is removed.
      @param degrees angle tolerance */
      void simplifyAngular(float degrees);

      /// Transforms the points on the path with the given matrix
      void transform(const Nimble::Matrix3f & m);

      /// Returns the number of points on the path
      size_t size() const { return m_points.size(); }
      /// Returns the ith point on the path
      Nimble::Vector2f point(size_t i) const { return m_points[i]; }

      /// Returns the center of the path computed from the point average.
      Nimble::Vector2f center() const;

      /// Tests whether two transformed paths intersect
      static bool intersect(const Path & p1, const Nimble::Matrix3f & m1, const Path & p2, const Nimble::Matrix3f & m2);

      /// Returns true if the path only contains two points that are very close to each other
      bool isDegenerate() const;

      /// Returns the points that make up the path
      container points() { return m_points; }

    private:
      container m_points;
  };

}

#endif
