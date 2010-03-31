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

#include "Plane.hpp"

/// @todo use Nimble::TOLERANCE
static const float EPSILON = 1e-5f;

namespace Nimble
{

  Plane::Plane()
  {}

  Plane::Plane(const Nimble::Vector3f & normal, float constant)
  : m_normal(normal),
    m_constant(constant)
  {}

  Plane::Plane(const Nimble::Vector3f & normal, const Nimble::Vector3f & point)
  : m_normal(normal)
  {
    m_constant = ::dot(normal, point);
  }

  float Plane::distanceTo(const Nimble::Vector3f & point) const
  {
    return ::dot(m_normal, point) - m_constant;
  }

  bool Plane::intersect(const Nimble::Vector3f & rayO, const Nimble::Vector3f & rayD, float & rayT) const
  {
    float dotDN = ::dot(rayD, m_normal);
    float dist = distanceTo(rayO);

    if(fabs(dotDN) > EPSILON) {
      // Not parallel
      rayT = -dist / dotDN;

      return true;
    }

    if(fabs(dist) <= EPSILON) {
      // Parallel
      rayT = 0.f;
      return true;
    }
    
    return false;
  }

}

