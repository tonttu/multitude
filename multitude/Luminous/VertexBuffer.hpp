/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef LUMINOUS_VERTEX_BUFFER_HPP
#define LUMINOUS_VERTEX_BUFFER_HPP

#include <Luminous/Export.hpp>
#include <Luminous/Luminous.hpp>

#include <Luminous/GLResource.hpp>

#include <stdlib.h> // size_t

#define BUFFER_OFFSET(bytes) ((GLubyte *)0 + (bytes))

namespace Luminous
{

  /// Abstraction of the OpenGL Buffer Objects.
  /// BufferObject provides an abstraction for the Buffer Objects (vertex
  /// buffers, index buffers) in OpenGL.
  template<GLenum type>
  class LUMINOUS_API BufferObject : public Luminous::GLResource
    {
      public:

        /// The policy for mapping a buffer object
        enum AccessMode {
#ifndef LUMINOUS_OPENGLES

          /// The client may perform a read operation on the pointer while the
          /// buffer is mapped.
          READ_ONLY = GL_READ_ONLY,
          /// The client may perform a write operation on the pointer while the
          /// buffer is mapped.
          WRITE_ONLY = GL_WRITE_ONLY,
          /// The client may perform both read and write operation on the
          /// pointer while the buffer is mapped.
          READ_WRITE = GL_READ_WRITE
#else
                       READ_ONLY,
                       WRITE_ONLY,
                       READ_WRITE
#endif // LUMINOUS_OPENGLES

        };

        /// A hint for the GL implementation as how a buffer object's data will
        /// be accessed. It may significantly affect the buffer object
        /// performance, but does not constrain the actual usage of the data
        /// store.
        enum Usage {
          /// The buffer contents will be specified once, and used at most a few
          /// times as the source for GL drawing and image specification
          /// commands.
          STREAM_DRAW = GL_STREAM_DRAW,
#ifndef LUMINOUS_OPENGLES
          /// The buffer contents will be specified once by reading data from
          /// the GL and used at most a few times by the application.
          STREAM_READ = GL_STREAM_READ,
          /// The buffer contents will be specified once by reading data from
          /// the GL and used at most a few times as the source for GL drawing
          /// and image specification commands.
          STREAM_COPY = GL_STREAM_COPY,
#endif // LUMINOUS_OPENGLES

          /// The buffer contents will be specified once, and used many times as
          /// the source for GL drawing and image specification commands.
          STATIC_DRAW = GL_STATIC_DRAW,
#ifndef LUMINOUS_OPENGLES
          /// The buffer contents will be specified once by reading data from
          /// the GL, and used many times by the application.
          STATIC_READ = GL_STATIC_READ,
          /// The buffer contents will be specified once by reading data from
          /// the GL, and used many times as the source for GL drawing and image
          /// specification commands.
          STATIC_COPY = GL_STATIC_COPY,
#endif // LUMINOUS_OPENGLES

          /// The buffer contents will be specified repeatedly, and used many
          // /times as the source for GL drawing and image specification commands.
          DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
#ifndef LUMINOUS_OPENGLES
          /// The buffer contents will be specified repeatedly by reading data
          /// from the GL, and used many times by the application.
          DYNAMIC_READ = GL_DYNAMIC_READ,
          /// The buffer contents will be specified repeatedly by reading data
          /// from the GL, and used many times as the source for GL drawing and
          /// image specification commands.
          DYNAMIC_COPY = GL_DYNAMIC_COPY
#endif // LUMINOUS_OPENGLES
        };

        /// Creates an empty OpenGL buffer object.
        BufferObject(Luminous::RenderContext * resources = 0);
        virtual ~BufferObject();

        /// Allocates memory for the vertex buffer
        void allocate(size_t bytes, Usage usage);

        /// Binds the vertex buffer
        void bind();
        /// Unbinds any vertex buffers
        void unbind();

        /// Fills the vertex buffer with data
        void fill(void * data, size_t bytes, Usage usage);
        /// Fills a part of the vertex buffer with data
        void partialFill(size_t start, void * data, size_t count);

#ifndef LUMINOUS_OPENGLES

        /// Maps the vertex buffer to CPU memory. The pointer is valid until unmap() is called.
        void * map(AccessMode mode);
        /// Unmaps the vertex buffer from CPU memory. The pointer to the buffer is invalidated.
        void unmap();
#endif // LUMINOUS_OPENGLES

        /** Access the OpenGL handle id.

            This function should be used with care, since it may break the OpenGL state tracking.

            @return Returns the OpenGL handle for the buffer
         */
        GLuint handle() const { return m_bufferId; }

        /** @return Returns the current number of filled bytes in the boffer. */
        size_t filled() const { return m_filled; }
      private:
        /// OpenGL handle for the vertex buffer
        GLuint m_bufferId;
        size_t m_filled;
        /// Is the buffer bound or not
        bool   m_bound;
    };

  /// An OpenGL vertex buffer

  class VertexBuffer : public BufferObject<GL_ARRAY_BUFFER>
  {
  public:
    /// Constructs an empty vertex buffer.
    VertexBuffer(Luminous::RenderContext * resources = 0)
      : BufferObject<GL_ARRAY_BUFFER>(resources)
    {}
  };

  /// An OpenGL index buffer
  class IndexBuffer : public BufferObject<GL_ELEMENT_ARRAY_BUFFER>
  {
  public:
    /// Constructs an empty index buffer.
    IndexBuffer(Luminous::RenderContext * resources = 0)
      : BufferObject<GL_ELEMENT_ARRAY_BUFFER>(resources)
    {}

  };

}

#endif
