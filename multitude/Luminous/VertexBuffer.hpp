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

#define BUFFER_OFFSET(bytes) ((GLubyte *)0 + (bytes))

namespace Luminous
{

  /// Abstraction of the OpenGL Buffer Objects.
  /// BufferObject provides an abstraction for the Buffer Objects (vertex
  /// buffers, index buffers) in OpenGL.
  template<GLenum type>
    class LUMINOUS_API BufferObject 
    {
      public:

        /// The policy for mapping a buffer object
        enum AccessMode {
          /// The client may perform a read operation on the pointer while the
          /// buffer is mapped.
          READ_ONLY = GL_READ_ONLY,
          /// The client may perform a write operation on the pointer while the
          /// buffer is mapped.
          WRITE_ONLY = GL_WRITE_ONLY,
          /// The client may perform both read and write operation on the
          /// pointer while the buffer is mapped.
          READ_WRITE = GL_READ_WRITE
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
          /// The buffer contents will be specified once by reading data from
          /// the GL and used at most a few times by the application.
          STREAM_READ = GL_STREAM_READ,
          /// The buffer contents will be specified once by reading data from
          /// the GL and used at most a few times as the source for GL drawing
          /// and image specification commands.
          STREAM_COPY = GL_STREAM_COPY,

          /// The buffer contents will be specified once, and used many times as
          /// the source for GL drawing and image specification commands.
          STATIC_DRAW = GL_STATIC_DRAW,
          /// The buffer contents will be specified once by reading data from
          /// the GL, and used many times by the application.
          STATIC_READ = GL_STATIC_READ,
          /// The buffer contents will be specified once by reading data from
          /// the GL, and used many times as the source for GL drawing and image
          /// specification commands.
          STATIC_COPY = GL_STATIC_COPY,

          /// The buffer contents will be specified repeatedly, and used many
          // /times as the source for GL drawing and image specification commands.
          DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
          /// The buffer contents will be specified repeatedly by reading data
          /// from the GL, and used many times by the application.
          DYNAMIC_READ = GL_DYNAMIC_READ,
          /// The buffer contents will be specified repeatedly by reading data
          /// from the GL, and used many times as the source for GL drawing and
          /// image specification commands.
          DYNAMIC_COPY = GL_DYNAMIC_COPY
        };

        BufferObject();
        ~BufferObject();

        void allocate(size_t bytes, Usage usage);

        void bind() const;
        void unbind() const;

        void fill(void * data, size_t bytes, Usage usage);
        void partialFill(size_t start, void * data, size_t count);

        void * map(AccessMode mode);
        void unmap();

      protected:
        GLuint m_bufferId;
    };

  typedef BufferObject<GL_ARRAY_BUFFER> VertexBuffer;
  typedef BufferObject<GL_ELEMENT_ARRAY_BUFFER> IndexBuffer;
  /*
#ifdef WIN32
#ifdef LUMINOUS_EXPORT
  // In WIN32 template classes must be instantiated to be exported
  template class BufferObject<GL_ARRAY_BUFFER>;
  template class BufferObject<GL_ELEMENT_ARRAY_BUFFER>;
#endif
#endif
*/
}

#endif
