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

  GLenum GLUtils::getBufferUsage(BufferUsage usage)
  {
    switch (usage)
    {
    case BU_Static_Copy:   return GL_STATIC_COPY;
    case BU_Static_Read:   return GL_STATIC_READ;
    case BU_Static_Write:  return GL_STATIC_DRAW;
    case BU_Dynamic_Copy:  return GL_DYNAMIC_COPY;
    case BU_Dynamic_Read:  return GL_DYNAMIC_READ;
    case BU_Dynamic_Write: return GL_DYNAMIC_DRAW;
    case BU_Stream_Copy:   return GL_STREAM_COPY;
    case BU_Stream_Read:   return GL_STREAM_READ;
    case BU_Stream_Write:  return GL_STREAM_DRAW;
    default:
      Radiant::warning("GLUtils: Unknown usage option (%d)", usage);
      assert(false);
      return GL_DYNAMIC_DRAW;
    }
  }

  GLenum GLUtils::getBufferType(BufferType type)
  {
    switch (type)
    {
    case BT_VertexBuffer: return GL_ARRAY_BUFFER;
    case BT_IndexBuffer: return GL_ELEMENT_ARRAY_BUFFER;
    case BT_ConstantBuffer: return GL_UNIFORM_BUFFER_EXT;
    default:
      Radiant::error("GLUtils: Unknown buffer type (%d)", type);
      assert(false);
      return GL_ARRAY_BUFFER;
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
}