#include "Luminous/GLUtils.hpp"
#include "Luminous/HardwareBuffer.hpp"

#include <cassert>

// Since we got rid of GLEW...
#ifdef RADIANT_OSX
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#endif

namespace Luminous
{  
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
    case ShaderType_Vertex:   return GL_VERTEX_SHADER;
    case ShaderType_Fragment: return GL_FRAGMENT_SHADER;
    case ShaderType_Geometry: return GL_GEOMETRY_SHADER_EXT;
    default:
      Radiant::error("GLUtils: cannot determine shader type (%d)", type);
      assert(false);
      return GL_VERTEX_SHADER;
    }
  }

  GLenum GLUtils::getUsageFlags(const HardwareBuffer & buffer)
  {
    switch (buffer.usage())
    {
    case BufferUsage_Static_Draw:  return GL_STATIC_DRAW;
    case BufferUsage_Static_Read:   return GL_STATIC_READ;
    case BufferUsage_Static_Copy:   return GL_STATIC_COPY;

    case BufferUsage_Stream_Draw:  return GL_STREAM_DRAW;
    case BufferUsage_Stream_Read:   return GL_STREAM_READ;
    case BufferUsage_Stream_Copy:   return GL_STREAM_COPY;

    case BufferUsage_Dynamic_Draw: return GL_DYNAMIC_DRAW;
    case BufferUsage_Dynamic_Read:  return GL_DYNAMIC_READ;
    case BufferUsage_Dynamic_Copy:  return GL_DYNAMIC_COPY;

    default:
      assert(false);
      Radiant::error("GLUtils: Unknown buffer usage flag %d", buffer.usage());
      return GL_STATIC_DRAW;
    };
  }

  GLuint GLUtils::createResource(ResourceType type)
  {
    GLuint resource;
    switch (type)
    {
    case ResourceType_VertexArray:    glGenVertexArrays(1, &resource); return resource;
    case ResourceType_Buffer:         glGenBuffers(1, &resource); return resource;
    case ResourceType_ShaderProgram:  return glCreateProgram();
    case ResourceType_VertexShader:   return glCreateShader(GL_VERTEX_SHADER);
    case ResourceType_FragmentShader: return glCreateShader(GL_FRAGMENT_SHADER);
    case ResourceType_GeometryShader: return glCreateShader(GL_GEOMETRY_SHADER_EXT);
    case ResourceType_Texture:        glGenTextures(1, & resource); return resource;
    default:
      Radiant::error("RenderDriverGL: Can't create GL resource: unknown type %d", type);
      assert(false);
      return 0;
    }
  }

  void GLUtils::destroyResource(ResourceType type, GLuint resource)
  {
    switch (type)
    {
    case ResourceType_VertexArray:    glDeleteVertexArrays(1, &resource); break;
    case ResourceType_Buffer:         glDeleteBuffers(1, &resource); break;
    case ResourceType_ShaderProgram:  return glDeleteProgram(resource); break;
    case ResourceType_VertexShader:   return glDeleteShader(resource); break;
    case ResourceType_FragmentShader: return glDeleteShader(resource); break;
    case ResourceType_GeometryShader: return glDeleteShader(resource); break;
    case ResourceType_Texture:        glDeleteTextures(1, & resource); break;
    default:
      Radiant::error("RenderDriverGL: Can't destroy GL resource: unknown type %d", type);
      assert(false);
    }
  }
}