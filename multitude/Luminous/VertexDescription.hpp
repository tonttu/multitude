#if !defined (LUMINOUS_VERTEXDESCRIPTION_HPP)
#define LUMINOUS_VERTEXDESCRIPTION_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/VertexAttribute.hpp"

#include <QString>
#include <vector>
#include <stdint.h>

namespace Luminous
{
  class VertexDescription
  {
  public:
    LUMINOUS_API VertexDescription();

    /// Add an attribute at the end
    template <typename AttrType> void addAttribute(const QString & attrName);
    /// Add an attribute at a specific offset
    template <typename AttrType> void addAttribute(const QString & attrName, uint offset);

    /// Tries to find a named attributes. Returns 0 if it's not found
    LUMINOUS_API const VertexAttribute * findAttribute(const QString & attrName) const;
    /// Remove a named attribute
    LUMINOUS_API void removeAttribute(const QString & attrName);

    /// Returns the number of attributes in this description
    LUMINOUS_API size_t attributeCount() const;
    /// Returns the requested attribute
    LUMINOUS_API VertexAttribute attribute(size_t index) const;
    /// Returns the total size of a vertex
    LUMINOUS_API uint vertexSize() const;
  private:
    template <typename T> void createAttribute(VertexAttribute & attr);
  private:
    std::vector<VertexAttribute> m_attributes;
  };

  template <typename AttrType> void VertexDescription::addAttribute(const QString & attrName, uint offset)
  {
    VertexAttribute attr;
    createAttribute<AttrType>(attr);
    attr.name = attrName;
    attr.offset = offset;
    m_attributes.push_back(attr);
  }

  template <typename AttrType> void VertexDescription::addAttribute(const QString & attrName)
  {
    addAttribute<AttrType>(attrName, vertexSize());
  }
}
#endif // LUMINOUS_VERTEXDESCRIPTION_HPP
