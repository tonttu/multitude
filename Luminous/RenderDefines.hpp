/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RENDERDEFINES_HPP
#define RENDERDEFINES_HPP

#include "Luminous.hpp"

namespace Luminous
{

  /// Defines which primitives are matched based on their facing.
  /// @sa FaceWinding
  enum Face
  {
    /// Front-facing primitives
    FRONT = GL_FRONT,
    /// Back-facing primitives
    BACK = GL_BACK,
    /// Front and back facing primitives
    FRONT_AND_BACK = GL_FRONT_AND_BACK
  };

  /// Defines the ordering of a rendering primitive
  enum FaceWinding
  {
    /// Counter clock-wise ordering of vertices
    CCW = GL_CCW,
    /// Clock-wise ordering of vertices
    CW = GL_CW
  };

}

#endif // RENDERDEFINES_HPP
