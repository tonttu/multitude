#if !defined (LUMINOUS_VERTEXDESCRIPTION_HPP)
#define LUMINOUS_VERTEXDESCRIPTION_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/VertexAttribute.hpp"

#include <QString>
#include <vector>
#include <cstdint>

namespace Luminous
{

  /// This class provides a description of vertex data format used during
  /// rendering. Vertex description maps the named vertex components (vertex
  /// attributes) used in shaders to data contained in vertex buffers.
  class VertexDescription
  {
  public:
    /// Construct an empty description
    LUMINOUS_API VertexDescription();

    /// Add an attribute at the end of the description
    /// @param attrName vertex attribute name
    template <typename AttrType>
    void addAttribute(const QString & attrName);
    /// Add an attribute at specific offset
    /// @param attrName vertex attribute name
    /// @param offset offset in bytes from the start of the vertex
    template <typename AttrType>
    void addAttribute(const QString & attrName, uint offset);

    /// Find a named attribute
    /// @param attrName vertex attribute name
    /// @return vertex attribute if the attribute is found; otherwise nullptr
    LUMINOUS_API const VertexAttribute * findAttribute(const QString & attrName) const;
    /// Remove a named attribute
    /// @param attrName vertex attribute name
    LUMINOUS_API void removeAttribute(const QString & attrName);

    /// Returns the number of attributes in this description
    /// @return number of attributes
    LUMINOUS_API size_t attributeCount() const;
    /// Returns the requested attribute
    /// @param index index of the vertex attribute
    /// @return copy of the requested vertex attribute
    LUMINOUS_API VertexAttribute attribute(size_t index) const;
    /// Returns the total size of a vertex
    /// @return vertex size in bytes
    LUMINOUS_API uint vertexSize() const;

  private:
    template <typename T>
    void createAttribute(VertexAttribute & attr);

    std::vector<VertexAttribute> m_attributes;
  };

  template <typename AttrType>
  void VertexDescription::addAttribute(const QString & attrName, uint offset)
  {
    VertexAttribute attr;
    createAttribute<AttrType>(attr);
    attr.name = attrName.toAscii();
    attr.offset = offset;
    m_attributes.push_back(attr);
  }

  template <typename AttrType>
  void VertexDescription::addAttribute(const QString & attrName)
  {
    addAttribute<AttrType>(attrName, vertexSize());
  }
}

#endif // LUMINOUS_VERTEXDESCRIPTION_HPP
