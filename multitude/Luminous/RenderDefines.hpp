#ifndef RENDERDEFINES_HPP
#define RENDERDEFINES_HPP

#include "Luminous.hpp"

namespace Luminous
{

  enum Face
  {
    Front = GL_FRONT,
    Back = GL_BACK,
    FrontAndBack = GL_FRONT_AND_BACK
  };

  enum FaceWinding
  {
    CCW = GL_CCW,
    CW = GL_CW
  };

}

#endif // RENDERDEFINES_HPP
