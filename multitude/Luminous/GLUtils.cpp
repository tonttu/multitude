#include "Luminous/GLUtils.hpp"
#include "Luminous/HardwareBuffer.hpp"

#include <cassert>

namespace Luminous
{
  GLenum GLUtils::getBufferLockOptions(BufferLockOptions lock)
  {
    switch (lock)
    {
    case BufferLockOptions_Read:      return GL_READ_ONLY;
    case BufferLockOptions_Write:     return GL_WRITE_ONLY;
    case BufferLockOptions_ReadWrite: return GL_READ_WRITE;
    default:
      Radiant::warning("GLUtils: Unknown lock option (%d)", lock);
      assert(false);
      return GL_READ_WRITE;
    }
  }
  
  /// Translate a PrimitiveType to its OpenGL equivalent
  GLenum GLUtils::getPrimitiveType(PrimitiveType type)
  {
    switch (type)
    {
    case PrimitiveType_Triangle:      return GL_TRIANGLES;
    case PrimitiveType_TriangleStrip: return GL_TRIANGLE_STRIP;
    case PrimitiveType_Line:          return GL_LINES;
    case PrimitiveType_LineStrip:     return GL_LINE_STRIP;;
    case PrimitiveType_Point:         return GL_POINT;
    default:
      Radiant::warning("GLUtils: Unknown primitive type (%d)", type);
      assert(false);
      return GL_TRIANGLES;
    }
  }
  
  GLenum GLUtils::getDataType(DataType type)
  {
    switch (type)
    {
    case DataType_Byte: return GL_BYTE;
    case DataType_Short: return GL_SHORT;
    case DataType_Int: return GL_INT;
    case DataType_UnsignedByte: return GL_UNSIGNED_BYTE;
    case DataType_UnsignedShort: return GL_UNSIGNED_SHORT;
    case DataType_UnsignedInt: return GL_UNSIGNED_INT;
    case DataType_Float: return GL_FLOAT;
    case DataType_Double: return GL_DOUBLE;
    default:
      Radiant::error("GLUtils: cannot determine data type (%d)", type);
      assert(false);
      return GL_FLOAT;
    }
  }

  GLenum GLUtils::getShaderType(ShaderType type)
  {
    switch (type)
    {
    case ShaderType_VertexShader:   return GL_VERTEX_SHADER;
    case ShaderType_FragmentShader: return GL_FRAGMENT_SHADER;
    case ShaderType_GeometryShader: return GL_GEOMETRY_SHADER_EXT;
    default:
      Radiant::error("GLUtils: cannot determine shader type (%d)", type);
      assert(false);
      return GL_VERTEX_SHADER;
    }
  }
}