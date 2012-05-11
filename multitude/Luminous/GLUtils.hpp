#if !defined (LUMINOUS_GLUTILS_HPP)
#define LUMINOUS_GLUTILS_HPP

#include "Luminous/Luminous.hpp"

namespace Luminous
{
  class GLUtils {
  public:
    static GLenum getBufferLockOptions(BufferLockOptions lock);
    static GLenum getBufferUsage(BufferUsage usage);
    static GLenum getBufferType(BufferType type);
    static GLenum getPrimitiveType(PrimitiveType type);
    static GLenum getDataType(DataType type);
    static GLenum getShaderType(ShaderType type);
  };
}

#endif // LUMINOUS_GLUTILS_HPP