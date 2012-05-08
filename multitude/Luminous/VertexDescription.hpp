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
    /// Add an attribute
    template <typename AttrType> void addAttribute(const QString & attrName, uint offset);
    /// Tries to find a named attributes. Returns 0 if it's not found
    LUMINOUS_API const VertexAttribute * findAttribute(const QString & attrName) const;
    /// Remove a named attribute
    LUMINOUS_API void removeAttribute(const QString & attrName);

    /// Returns the number of attributes in this description
    LUMINOUS_API size_t numAttributes() const;
    /// Returns the requested attribute
    LUMINOUS_API VertexAttribute attribute(size_t index) const;
    /// Returns the total size of a vertex
    LUMINOUS_API uint vertexSize() const;
  private:
    template <typename T> bool createAttribute(VertexAttribute & attr);

  private:
    std::vector<VertexAttribute> m_attributes;
  };

  template <typename T> bool createAttribute(VertexAttribute & attr)
  {
    Radiant::error("VertexDescription: Unable to create attribute: unsupported type");
    return false;
  }

  template <typename AttrType> void VertexDescription::addAttribute(const QString & attrName, uint offset)
  {
    VertexAttribute attr;
    if (createAttribute<AttrType>(attr)) {
      attr.name = attrName;
      attr.offset = offset;
      m_attributes.push_back(attr);
    }
  }
}
#endif // LUMINOUS_VERTEXDESCRIPTION_HPP
