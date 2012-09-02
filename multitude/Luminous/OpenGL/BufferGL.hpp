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

    void bind(Buffer::Type type);
    //void upload(Buffer &buffer);

    void * map(Buffer::Type type, int offset, std::size_t length, Radiant::FlagsT<Buffer::MapAccess> access);
    void unmap(Buffer::Type type, int offset = 0, std::size_t length = std::size_t(-1));

  private:
    void allocate(Buffer::Type type);

  private:
    Buffer::Usage m_usage;
    size_t m_size;        // Size (in bytes)
    size_t m_allocatedSize;
    size_t m_uploaded;    // Uploaded bytes (for incremental upload)
    int m_generation;
  };
}

#endif // LUMINOUS_TEXTUREGL_HPP
