#pragma once

#include "StateGL.hpp"
#include "Buffer.hpp"

namespace Luminous
{
  class UploadBuffer;
  /// A reference to a buffer object meant for uploading data to GPU. Perform
  /// all operations before the destructor is called, since that will add a new
  /// fence to the OpenGL command queue to track when the buffer can be reused.
  class UploadBufferRef
  {
  public:
    UploadBufferRef(UploadBuffer * uploadBuffer)
      : m_uploadBuffer(uploadBuffer)
    {}

    UploadBufferRef(const UploadBufferRef &) = delete;
    UploadBufferRef & operator=(const UploadBufferRef &) = delete;

    UploadBufferRef(UploadBufferRef && u)
      : m_uploadBuffer(u.m_uploadBuffer)
    {
      u.m_uploadBuffer = nullptr;
    }

    UploadBufferRef & operator=(UploadBufferRef && u)
    {
      std::swap(m_uploadBuffer, u.m_uploadBuffer);
      return *this;
    }

    LUMINOUS_API void bind(Buffer::Type type);
    LUMINOUS_API void upload(Buffer::Type type, int offset, std::size_t length, const void * data);
    LUMINOUS_API void * map(Buffer::Type type, int offset, std::size_t length, Radiant::FlagsT<Buffer::MapAccess> access);
    LUMINOUS_API void unmap(Buffer::Type type, int offset, std::size_t length);

    /// If supported by system, this returns write-only coherent and
    /// unsynchronized persistent mapping to the buffer. Use this for ideal
    /// performance.
    LUMINOUS_API void * persistentMapping();

    LUMINOUS_API ~UploadBufferRef();

  private:
    bool m_addFence = false;
    UploadBuffer * m_uploadBuffer = nullptr;
  };

  /// Pool memory allocator for reusable OpenGL buffer objects meant for
  /// uploading data to textures with GL_PIXEL_UNPACK_BUFFER. All functions
  /// need to be called from an active OpenGL context. All functions are
  /// thread-safe.
  class LUMINOUS_API UploadBufferPool
  {
  public:
    UploadBufferPool(StateGL & stateGl);
    ~UploadBufferPool();

    /// Allocate or reuse a buffer object that has at least the given size in bytes.
    UploadBufferRef allocate(size_t size);

    /// Preallocate some number of various size buffer objects so that the GPU
    /// memory size never goes over the given limit, in bytes. Allocating is
    /// slow and can cause jank if done lazily when needed, that's why it's
    /// more efficient to preallocate some amount of memory beforehand.
    void preallocate(size_t maxSize);
    /// Perform some garbage collection. Try to release memory so that the pool
    /// doesn't consume more than maxSize bytes on the GPU, and if there are
    /// buffers in the pool that haven't been used in a while, the function can
    /// release memory even more, until it no longer consumes more than
    /// targetSize bytes.
    void release(size_t targetSize, size_t maxSize);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
