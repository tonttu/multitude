#if !defined (LUMINOUS_VERTEXATTRIBUTE_HPP)
#define LUMINOUS_VERTEXATTRIBUTE_HPP

#include "Luminous/Luminous.hpp"
#include <QString>

namespace Luminous
{

  /// This class represents a single vertex attribute in vertex description.
  /// @sa VertexDescription
  struct VertexAttribute
  {
    enum Type
    {
      Byte          = GL_BYTE,
      Short         = GL_SHORT,
      Int           = GL_INT,
      Float         = GL_FLOAT,
      Double        = GL_DOUBLE,
      UnsignedByte  = GL_UNSIGNED_BYTE,
      UnsignedShort = GL_UNSIGNED_SHORT,
      UnsignedInt   = GL_UNSIGNED_INT,
    };

    VertexAttribute()
      : type(VertexAttribute::Float)
      , count(0)
      , offset(0)
      , size(0)
      , normalized(false)
    {
    }

    QByteArray name;
    Type type;
    uint count;
    uint offset;
    uint size;
    bool normalized;
  };
}

inline bool operator==(const Luminous::VertexAttribute & lhs, const Luminous::VertexAttribute & rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type && lhs.offset == rhs.offset && lhs.size == rhs.size && lhs.normalized == rhs.normalized;
}
inline bool operator!=(const Luminous::VertexAttribute & lhs, const Luminous::VertexAttribute & rhs) { return !(lhs == rhs); }

#endif // LUMINOUS_VERTEXATTRIBUTE_HPP
