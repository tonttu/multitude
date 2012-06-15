#if !defined (LUMINOUS_GLUTILS_HPP)
#define LUMINOUS_GLUTILS_HPP

#include "Luminous/Luminous.hpp"

namespace Luminous
{
  class GLUtils {
  public:
    LUMINOUS_API static GLenum getBufferLockOptions(BufferLockOptions lock);
    LUMINOUS_API static GLenum getPrimitiveType(PrimitiveType type);

    LUMINOUS_API static GLenum getDataType(DataType type);
    LUMINOUS_API static GLenum getShaderType(ShaderType type);
  };
}

#endif // LUMINOUS_GLUTILS_HPP
