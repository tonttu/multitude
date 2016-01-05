/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

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
    /// Data type for a component
    enum Type
    {
      Byte          = static_cast<unsigned int>(GL_BYTE),
      Short         = static_cast<unsigned int>(GL_SHORT),
      Int           = static_cast<unsigned int>(GL_INT),
      Float         = static_cast<unsigned int>(GL_FLOAT),
      Double        = static_cast<unsigned int>(GL_DOUBLE),
      UnsignedByte  = static_cast<unsigned int>(GL_UNSIGNED_BYTE),
      UnsignedShort = static_cast<unsigned int>(GL_UNSIGNED_SHORT),
      UnsignedInt   = static_cast<unsigned int>(GL_UNSIGNED_INT),
    };

    /// Construct an empty vertex attribute
    VertexAttribute()
      : type(VertexAttribute::Float)
      , count(0)
      , offset(0)
      , size(0)
      , normalized(false)
    {
    }

    /// Vertex attribute name
    QByteArray name;
    /// Data-type
    Type type;
    /// Data count
    uint count;
    /// Offset in bytes from the beginning of VertexArray
    uint offset;
    /// Size of the vertex attribute in bytes
    uint size;
    /// Is the attribute normalized
    bool normalized;
  };
}

inline bool operator==(const Luminous::VertexAttribute & lhs, const Luminous::VertexAttribute & rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type && lhs.offset == rhs.offset && lhs.size == rhs.size && lhs.normalized == rhs.normalized;
}
inline bool operator!=(const Luminous::VertexAttribute & lhs, const Luminous::VertexAttribute & rhs) { return !(lhs == rhs); }

#endif // LUMINOUS_VERTEXATTRIBUTE_HPP
