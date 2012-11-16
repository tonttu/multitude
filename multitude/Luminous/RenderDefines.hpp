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
    Front = GL_FRONT,
    /// Back-facing primitives
    Back = GL_BACK,
    /// Front and back facing primitives
    FrontAndBack = GL_FRONT_AND_BACK
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
