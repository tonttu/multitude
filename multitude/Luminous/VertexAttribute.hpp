#if !defined (LUMINOUS_VERTEXATTRIBUTE_HPP)
#define LUMINOUS_VERTEXATTRIBUTE_HPP

#include "Luminous/Luminous.hpp"
#include <QString>

namespace Luminous
{
  struct VertexAttribute
  {
    enum Type
    {
      Byte,
      Short,
      Int,
      Float,
      Double,
      UnsignedByte,
      UnsignedShort,
      UnsignedInt,
    };

    VertexAttribute()
      : type(VertexAttribute::Float)
      , count(0)
      , offset(0)
      , size(0)
      , normalized(false)
    {
    }

    QString name;
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