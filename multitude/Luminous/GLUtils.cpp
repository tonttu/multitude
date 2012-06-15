#include "Luminous/GLUtils.hpp"
#include "Luminous/HardwareBuffer.hpp"

#include <cassert>

namespace Luminous
{
  GLenum GLUtils::getBufferLockOptions(BufferLockOptions lock)
  {
    switch (lock)
    {
    case BLO_Read:  return GL_READ_ONLY;
    case BLO_Write: return GL_WRITE_ONLY;
    case BLO_ReadWrite: return GL_READ_WRITE;
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
    case PT_Triangle: return GL_TRIANGLES;
    case PT_TriangleStrip: return GL_TRIANGLE_STRIP;
    case PT_Line: return GL_LINES;
    case PT_LineStrip: return GL_LINE_STRIP;;
    case PT_Point: return GL_POINT;
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
    case DT_Byte: return GL_BYTE;
    case DT_Short: return GL_SHORT;
    case DT_Int: return GL_INT;
    case DT_UnsignedByte: return GL_UNSIGNED_BYTE;
    case DT_UnsignedShort: return GL_UNSIGNED_SHORT;
    case DT_UnsignedInt: return GL_UNSIGNED_INT;
    case DT_Float: return GL_FLOAT;
    case DT_Double: return GL_DOUBLE;
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
    case ST_VertexShader:   return GL_VERTEX_SHADER;
    case ST_FragmentShader: return GL_FRAGMENT_SHADER;
    case ST_GeometryShader: return GL_GEOMETRY_SHADER_EXT;
    default:
      Radiant::error("GLUtils: cannot determine shader type (%d)", type);
      assert(false);
      return GL_VERTEX_SHADER;
    }
  }
}