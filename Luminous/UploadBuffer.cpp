/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "BufferGL.hpp"
#include "UploadBuffer.hpp"

#include <Radiant/Mutex.hpp>

#include <boost/container/flat_map.hpp>

// #define TRACK_ALLOCATIONS

#ifdef RADIANT_MACOS
#define GL_DYNAMIC_STORAGE_BIT            0x0100
#define GL_MAP_PERSISTENT_BIT             0x0040
#define GL_MAP_COHERENT_BIT               0x0080
#endif

namespace Luminous
{
  class UploadBuffer
  {
  public:
    UploadBuffer(StateGL & stateGl, size_t size, bool inUse)
      : m_stateGl(stateGl)
      , m_buffer(stateGl, Buffer::DYNAMIC_DRAW)
      , m_inUse(inUse)
    {
      GLbitfield allocateFlags = GL_DYNAMIC_STORAGE_BIT
          | GL_MAP_WRITE_BIT
          | GL_MAP_PERSISTENT_BIT
          | GL_MAP_COHERENT_BIT;
      if (m_buffer.allocateImmutable(size, allocateFlags)) {
        m_immutableAllocation = true;
      } else {
        m_buffer.bind(Buffer::UNPACK);
        m_buffer.allocate(Buffer::UNPACK, size);
        m_buffer.unbind(Buffer::UNPACK);
      }
      m_buffer.setExpirationSeconds(5);
    }

    ~UploadBuffer()
    {
      if (m_mapped)
        m_stateGl.opengl45()->glUnmapNamedBuffer(m_buffer.handle());
    }

    UploadBuffer(UploadBuffer && b) = delete;
    UploadBuffer & operator=(UploadBuffer && b) = delete;

    void release(bool addFence)
    {
      if (addFence)
        m_sync = m_stateGl.opengl().glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      m_inUse = false;
    }

    bool isInUse()
    {
      if (m_inUse)
        return true;

      if (m_sync) {
        GLenum r = m_stateGl.opengl().glClientWaitSync(m_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
        if (r == GL_ALREADY_SIGNALED || r == GL_CONDITION_SATISFIED) {
          m_stateGl.opengl().glDeleteSync(m_sync);
          m_sync = nullptr;
        }
      }

      return m_sync;
    }

  public:
    StateGL & m_stateGl;
    BufferGL m_buffer;
    std::atomic<bool> m_inUse{true};
    GLsync m_sync = nullptr;
    void * m_mapped = nullptr;
    bool m_immutableAllocation = false;
  };

  /////////////////////////////////////////////////////////////////////////////

  class UploadBufferPool::D
  {
  public:
    D(StateGL & stateGl)
      : m_stateGl(stateGl)
    {}

  public:
    StateGL & m_stateGl;
    Radiant::Mutex m_poolMutex;
    boost::container::flat_multimap<size_t, std::unique_ptr<UploadBuffer>> m_buffers;
    size_t m_totalSize = 0;
  };

  /////////////////////////////////////////////////////////////////////////////

  UploadBufferPool::UploadBufferPool(StateGL & stateGl)
    : m_d(new D(stateGl))
  {
  }

  UploadBufferPool::~UploadBufferPool()
  {
  }

  UploadBufferRef UploadBufferPool::allocate(size_t size)
  {
    size_t reservedSize = 1 << 16;
    while (reservedSize < size)
      reservedSize <<= 1;
    size_t maxSize = reservedSize << 2;

    Radiant::Guard g(m_d->m_poolMutex);
    for (auto & p: m_d->m_buffers) {
      if (p.first >= size && p.first <= maxSize && !p.second->isInUse()) {
        p.second->m_inUse = true;
        return p.second.get();
      }
    }

    m_d->m_totalSize += reservedSize;
    auto buffer = std::make_unique<UploadBuffer>(m_d->m_stateGl, reservedSize, true);
    auto it = m_d->m_buffers.emplace(reservedSize, std::move(buffer));
#ifdef TRACK_ALLOCATIONS
    Radiant::info("UploadBufferPool::allocate # %.1f kB [new pool size: %.2f MB]",
                  reservedSize / 1024.0, m_d->m_totalSize / 1024.0 / 1024.0);
#endif
    return it->second.get();
  }

  void UploadBufferPool::preallocate(size_t maxSize)
  {
    if (m_d->m_totalSize >= maxSize)
      return;

    size_t nextAllocation = 1 << 16;
    size_t toAllocate = maxSize - m_d->m_totalSize;

    Radiant::Guard g(m_d->m_poolMutex);
    while (nextAllocation <= toAllocate) {
      bool found = false;
      for (auto & p: m_d->m_buffers) {
        if (p.first == nextAllocation) {
          found = true;
          break;
        }
      }
      if (!found) {
        auto buffer = std::make_unique<UploadBuffer>(m_d->m_stateGl, nextAllocation, false);
        m_d->m_buffers.emplace(nextAllocation, std::move(buffer));
        m_d->m_totalSize += nextAllocation;
        toAllocate -= nextAllocation;

#ifdef TRACK_ALLOCATIONS
        Radiant::info("UploadBufferPool::preallocate # %.1f kB [new pool size: %.2f MB]",
                      nextAllocation / 1024.0, m_d->m_totalSize / 1024.0 / 1024.0);
#endif
      }
      nextAllocation <<= 1;
    }
  }

  void UploadBufferPool::release(size_t targetSize, size_t maxSize)
  {
    std::vector<std::unique_ptr<UploadBuffer>> toRelease;

    {
      Radiant::Guard g(m_d->m_poolMutex);
      for (int i = 0; i < 2; ++i) {
        for (auto it = m_d->m_buffers.end(); it != m_d->m_buffers.begin();) {
          if (m_d->m_totalSize <= targetSize)
            return;

          --it;
          std::unique_ptr<UploadBuffer> & b = it->second;
          if (b->isInUse())
            continue;

          if (b->m_buffer.expired() || (i == 1 && m_d->m_totalSize > maxSize)) {
#ifdef TRACK_ALLOCATIONS
            Radiant::info("UploadBufferPool::release # %.1f kB [new pool size: %.2f MB]",
                          it->first / -1024.0, m_d->m_totalSize / 1024.0 / 1024.0);
#endif
            m_d->m_totalSize -= it->first;
            toRelease.push_back(std::move(b));
            it = m_d->m_buffers.erase(it);
          }
        }
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  void UploadBufferRef::bind(Buffer::Type type)
  {
    m_uploadBuffer->m_buffer.bind(type);
  }

  void UploadBufferRef::upload(Buffer::Type type, int offset, size_t length, const void * data)
  {
    m_uploadBuffer->m_buffer.upload(type, offset, length, data);
  }

  void * UploadBufferRef::map(Buffer::Type type, int offset, size_t length, Radiant::FlagsT<Buffer::MapAccess> access)
  {
    auto data = m_uploadBuffer->m_buffer.map(type, offset, length, access);
    if ((access & Buffer::MAP_UNSYNCHRONIZED) && data)
      m_addFence = true;
    return data;
  }

  void UploadBufferRef::unmap(Buffer::Type type, int offset, size_t length)
  {
    m_uploadBuffer->m_buffer.unmap(type, offset, length);
  }

  void * UploadBufferRef::persistentMapping()
  {
    if (!m_uploadBuffer->m_immutableAllocation)
      return nullptr;

    if (!m_uploadBuffer->m_mapped) {
      GLbitfield mapFlags = GL_MAP_WRITE_BIT
          | GL_MAP_PERSISTENT_BIT
          | GL_MAP_COHERENT_BIT
          | GL_MAP_UNSYNCHRONIZED_BIT;
      auto & buffer = m_uploadBuffer->m_buffer;
      m_uploadBuffer->m_mapped = m_uploadBuffer->m_stateGl.opengl45()->glMapNamedBufferRange(
            buffer.handle(), 0, static_cast<GLsizei>(buffer.allocatedSize()), mapFlags);
    }

    auto mapped = m_uploadBuffer->m_mapped;
    if (mapped)
      m_addFence = true;
    return mapped;
  }

  UploadBufferRef::~UploadBufferRef()
  {
    if (m_uploadBuffer)
      m_uploadBuffer->release(m_addFence);
  }
}
