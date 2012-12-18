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
