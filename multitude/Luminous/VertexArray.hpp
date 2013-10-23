/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_VERTEX_ARRAY_HPP)
#define LUMINOUS_VERTEX_ARRAY_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"
#include "Luminous/VertexDescription.hpp"

#include <memory>
#include <vector>

namespace Luminous
{

  /// This class abstracts OpenGL VertexArrayObjects. GPU correspondent of this class is
  /// VertexArrayGL
  /// @todo implement copying (note bindings)
  class VertexArray
    : public RenderResource
    , public Patterns::NotCopyable
  {
  public:
    /// Stores vertex description and id of buffer containing vertices
    struct Binding
    {
      /// Buffer id
      // This can't be a raw pointer, since we will std::move the original buffer
      RenderResource::Id buffer;
      /// Description of vertex data
      Luminous::VertexDescription description;
      /// Compares buffer ids
      bool operator==(RenderResource::Id id) const { return buffer==id; }
    };

  public:
#ifdef RADIANT_DELETED_CONSTRUCTORS
    VertexArray(const VertexArray &) = delete;
#endif
    /// Constructor of VertexArray
    LUMINOUS_API VertexArray();
    /// Destroctor of VertexArray, doesn't destroy any buffers attached
    LUMINOUS_API ~VertexArray();

    /// Move constructor
    /// @param b VertexArray to move
    LUMINOUS_API VertexArray(VertexArray && b);
    /// Move assignment operator
    /// @param b VertexArrau to move
    /// @return Reference to this
    LUMINOUS_API VertexArray & operator=(VertexArray && b);

    /// Adds binding for the given vertex buffer
    /// @param vertexBuffer Vertex buffer which contains vertex data conforming to description
    /// @param description Description of vertex data in vertexBuffer
    LUMINOUS_API void addBinding(const Luminous::Buffer & vertexBuffer, const Luminous::VertexDescription & description);
    /// Sets index buffer for this vertex array
    /// @param indexBuffer Buffer storing indices
    LUMINOUS_API void setIndexBuffer(const Luminous::Buffer & indexBuffer);
    /// Removes binding for the given buffer
    /// @param buffer Buffer to remove
    LUMINOUS_API void removeBinding(const Luminous::Buffer & buffer);
    /// Removes all bindings
    LUMINOUS_API void clear();

    /// How many bindings is stored in this object
    /// @return Number of bindings
    LUMINOUS_API size_t bindingCount() const;
    /// Returns binding according based on their ordering (first added binding has index 0, second 1, etc)
    /// @param index Index for the binding
    /// @return Binding corresponding the given index
    LUMINOUS_API const Binding & binding(size_t index) const;
    /// Returns index buffer of this vertex array
    /// @return Id of the index buffer
    LUMINOUS_API RenderResource::Id indexBuffer() const;
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_VERTEX_ARRAY_HPP
