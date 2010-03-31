/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef NIMBLE_PLANE_HPP
#define NIMBLE_PLANE_HPP

#include <Nimble/Vector3.hpp>

namespace Nimble
{

  /// Plane is represented by dot(N, X) = c, where N is a unit-length normal
  /// vector, c is the plane constant and X is any point on the plane. User must
  /// ensure the normal vector satisfied this condition.
  class NIMBLE_API Plane
  {
  public:
    Plane();
    Plane(const Nimble::Vector3f & normal, float constant);
    Plane(const Nimble::Vector3f & normal, const Nimble::Vector3f & point);
  
    /// Compute the signed distance to the plane. Distance is positive if the
    /// point is on the positive side of the plane negative if vice-versa.
    float distanceTo(const Nimble::Vector3f & point) const;

    /// Does a ray intersect the plane?
    /// @param rayO ray origin
    /// @param rayD unit-length ray direction vector
    /// @param rayT how far the intersection is from the ray origin
    /// @return true if the ray intersects the plane, false otherwise
    bool intersect(const Nimble::Vector3f & rayO, const Nimble::Vector3f & rayD, float & rayT) const;

    private:
      Nimble::Vector3f m_normal;
      float m_constant;     
  };

}

#endif
