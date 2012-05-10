#include "Luminous/VertexDescription.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>
#include <cassert>

namespace Luminous
{
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
    uint maxOffset = 0;
    uint size = 0;

    for (size_t i = 0; i < m_attributes.size(); ++i) {
      if (m_attributes[i].offset >= maxOffset) {
        maxOffset = m_attributes[i].offset;
        size = m_attributes[i].offset + m_attributes[i].count * Luminous::Utils2::getDataSize(m_attributes[i].type);
      }
    }
    return size;
  }

  /// Specializations to create the vertex attributes
  template <> LUMINOUS_API bool createAttribute<char>(VertexAttribute & attr) { attr.type = DT_Byte; attr.count = 1; return true; }
  template <> LUMINOUS_API bool createAttribute<short>(VertexAttribute & attr) { attr.type = DT_Short; attr.count = 1; return true; }
  template <> LUMINOUS_API bool createAttribute<int>(VertexAttribute & attr) { attr.type = DT_Float; attr.count = 1; return true; }
  template <> LUMINOUS_API bool createAttribute<float>(VertexAttribute & attr) { attr.type = DT_Float; attr.count = 1; return true; }
  template <> LUMINOUS_API bool createAttribute<double>(VertexAttribute & attr) { attr.type = DT_Float; attr.count = 1; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector2i>(VertexAttribute & attr) { attr.type = DT_Int; attr.count = 2; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector3i>(VertexAttribute & attr) { attr.type = DT_Int; attr.count = 3; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector4i>(VertexAttribute & attr) { attr.type = DT_Int; attr.count = 4; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector2f>(VertexAttribute & attr) { attr.type = DT_Float; attr.count = 2; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector3f>(VertexAttribute & attr) { attr.type = DT_Float; attr.count = 3; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector4f>(VertexAttribute & attr) { attr.type = DT_Float; attr.count = 4; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector2d>(VertexAttribute & attr) { attr.type = DT_Double; attr.count = 2; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector3d>(VertexAttribute & attr) { attr.type = DT_Double; attr.count = 3; return true; }
  template <> LUMINOUS_API bool createAttribute<Nimble::Vector4d>(VertexAttribute & attr) { attr.type = DT_Double; attr.count = 4; return true; }
}
