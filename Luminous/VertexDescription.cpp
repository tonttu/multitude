/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/VertexDescription.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>
#include <cassert>

namespace Luminous
{
  VertexDescription::VertexDescription()
  {
  }

  const VertexAttribute * VertexDescription::findAttribute(const QString & attrName) const
  {
    for (size_t i = 0; i < m_attributes.size(); ++i) {
      if (m_attributes[i].name == attrName)
        return &m_attributes[i];
    }
    return 0;
  }

  void VertexDescription::removeAttribute(const QString & attrName)
  {
    for (size_t i = 0; i < m_attributes.size(); ++i) {
      if (m_attributes[i].name == attrName) {
        m_attributes.erase(m_attributes.begin() + i);
        break;
      }
    }
  }

  size_t VertexDescription::attributeCount() const
  {
    return m_attributes.size();
  }

  VertexAttribute VertexDescription::attribute(size_t index) const
  {
    assert(index < attributeCount());  // Out of bounds check
    return m_attributes[index];
  }

  uint VertexDescription::vertexSize() const
  {
    uint size = 0;
    for (size_t i = 0; i < m_attributes.size(); ++i)
      size = std::max(size,m_attributes[i].offset + m_attributes[i].size);
    return size;
  }

  /// Specializations to create the vertex attributes
  template <> LUMINOUS_API void VertexDescription::createAttribute<char>(VertexAttribute & attr) { attr.type = VertexAttribute::Byte; attr.count = 1; attr.size = sizeof(char); }
  template <> LUMINOUS_API void VertexDescription::createAttribute<short>(VertexAttribute & attr) { attr.type = VertexAttribute::Short; attr.count = 1; attr.size = sizeof(short); }
  template <> LUMINOUS_API void VertexDescription::createAttribute<int>(VertexAttribute & attr) { attr.type = VertexAttribute::Int; attr.count = 1; attr.size = sizeof(int); }
  template <> LUMINOUS_API void VertexDescription::createAttribute<float>(VertexAttribute & attr) { attr.type = VertexAttribute::Float; attr.count = 1; attr.size = sizeof(float); }
  template <> LUMINOUS_API void VertexDescription::createAttribute<double>(VertexAttribute & attr) { attr.type = VertexAttribute::Double; attr.count = 1; attr.size = sizeof(double); }
  template <> LUMINOUS_API void VertexDescription::createAttribute<unsigned char>(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedByte; attr.count = 1; attr.size = sizeof(unsigned char); }
  template <> LUMINOUS_API void VertexDescription::createAttribute<unsigned short>(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedShort; attr.count = 1; attr.size = sizeof(unsigned short); }
  template <> LUMINOUS_API void VertexDescription::createAttribute<unsigned int>(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedInt; attr.count = 1; attr.size = sizeof(unsigned int); }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector2i>(VertexAttribute & attr) { attr.type = VertexAttribute::Int; attr.count = 2; attr.size = sizeof(int)*2; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector3i>(VertexAttribute & attr) { attr.type = VertexAttribute::Int; attr.count = 3; attr.size = sizeof(int)*3; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector4i>(VertexAttribute & attr) { attr.type = VertexAttribute::Int; attr.count = 4; attr.size = sizeof(int)*4; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector2f>(VertexAttribute & attr) { attr.type = VertexAttribute::Float; attr.count = 2; attr.size = sizeof(float)*2; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector3f>(VertexAttribute & attr) { attr.type = VertexAttribute::Float; attr.count = 3; attr.size = sizeof(float)*3; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector4f>(VertexAttribute & attr) { attr.type = VertexAttribute::Float; attr.count = 4; attr.size = sizeof(float)*4; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector2d>(VertexAttribute & attr) { attr.type = VertexAttribute::Double; attr.count = 2; attr.size = sizeof(double)*2; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector3d>(VertexAttribute & attr) { attr.type = VertexAttribute::Double; attr.count = 3; attr.size = sizeof(double)*3; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector4d>(VertexAttribute & attr) { attr.type = VertexAttribute::Double; attr.count = 4; attr.size = sizeof(double)*4; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector2T<unsigned char> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedByte; attr.count = 2; attr.size = sizeof(unsigned char)*2; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector3T<unsigned char> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedByte; attr.count = 3; attr.size = sizeof(unsigned char)*3; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector4T<unsigned char> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedByte; attr.count = 4; attr.size = sizeof(unsigned char)*4; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector2T<unsigned short> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedShort; attr.count = 2; attr.size = sizeof(unsigned short)*2; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector3T<unsigned short> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedShort; attr.count = 3; attr.size = sizeof(unsigned short)*3; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector4T<unsigned short> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedShort; attr.count = 4; attr.size = sizeof(unsigned short)*4; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector2T<unsigned int> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedInt; attr.count = 2; attr.size = sizeof(unsigned int)*2; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector3T<unsigned int> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedInt; attr.count = 3; attr.size = sizeof(unsigned int)*3; }
  template <> LUMINOUS_API void VertexDescription::createAttribute<Nimble::Vector4T<unsigned int> >(VertexAttribute & attr) { attr.type = VertexAttribute::UnsignedInt; attr.count = 4; attr.size = sizeof(unsigned int)*4; }
}
