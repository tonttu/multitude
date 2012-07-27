#ifndef LUMINOUS_BUFFERGL_HPP
#define LUMINOUS_BUFFERGL_HPP

#include "ResourceHandleGL.hpp"
#include "Luminous/Buffer.hpp"

namespace Luminous
{
  class BufferGL : public ResourceHandleGL
  {
  public:
    BufferGL(StateGL & state, const Buffer & buffer);
    ~BufferGL();

    BufferGL(BufferGL && t);
    BufferGL & operator=(BufferGL && t);

    void bind();
    void upload(Buffer &buffer);

    void * map(int offset, std::size_t length, Radiant::FlagsT<Buffer::MapAccess> access);
    void unmap();

    /// @todo hack, remove
    GLuint handle() const { return m_handle; }

  private:
    void allocate();

  private:
    Buffer::Usage m_usage;
    size_t m_size;        // Size (in bytes)
    size_t m_allocatedSize;
    size_t m_uploaded;    // Uploaded bytes (for incremental upload)
    GLenum m_target;
    int m_generation;
  };
}

#endif // LUMINOUS_TEXTUREGL_HPP
