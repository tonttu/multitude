/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Plane.hpp"

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
    m_constant = dot(normal, point);
  }

  float Plane::distanceTo(const Nimble::Vector3f & point) const
  {
    return dot(m_normal, point) - m_constant;
  }

  bool Plane::intersect(const Nimble::Vector3f & rayO, const Nimble::Vector3f & rayD, float & rayT) const
  {
    auto dotDN = dot(rayD, m_normal);
    auto dist = distanceTo(rayO);

    if(std::abs(dotDN) > std::numeric_limits<float>::epsilon()) {
      // Not parallel
      rayT = -dist / dotDN;

      return true;
    }

    if(std::abs(dist) <= std::numeric_limits<float>::epsilon()) {
      // Parallel
      rayT = 0.f;
      return true;
    }
    
    return false;
  }

}

