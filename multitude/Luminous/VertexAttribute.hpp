#if !defined (LUMINOUS_VERTEXATTRIBUTE_HPP)
#define LUMINOUS_VERTEXATTRIBUTE_HPP

#include "Luminous/Luminous.hpp"
#include <QString>

namespace Luminous
{
  struct VertexAttribute
  {
    VertexAttribute()
      : type(Luminous::DataType_Float)
      , offset(0)
      , count(0)
      , normalized(false)
    {
    }

    QString name;
    DataType type;
    uint offset;
    uint count;
    bool normalized;
  };
}

inline bool operator==(const Luminous::VertexAttribute & lhs, const Luminous::VertexAttribute & rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type && lhs.offset == rhs.offset && lhs.count == rhs.count && lhs.normalized == rhs.normalized;
}
inline bool operator!=(const Luminous::VertexAttribute & lhs, const Luminous::VertexAttribute & rhs) { return !(lhs == rhs); }

#endif // LUMINOUS_VERTEXATTRIBUTE_HPP