#if !defined (LUMINOUS_GLUTILS_HPP)
#define LUMINOUS_GLUTILS_HPP

#include "Luminous/Luminous.hpp"

namespace Luminous
{
  class GLUtils {
  public:
    LUMINOUS_API static GLenum getPrimitiveType(PrimitiveType type);

    LUMINOUS_API static GLenum getDataType(DataType type);
    LUMINOUS_API static GLenum getShaderType(ShaderType type);

    LUMINOUS_API static GLenum getUsageFlags(const HardwareBuffer & buffer);

    LUMINOUS_API static GLuint createResource(ResourceType type);
    LUMINOUS_API static void destroyResource(ResourceType type, GLuint resource);
  };
}

#endif // LUMINOUS_GLUTILS_HPP
