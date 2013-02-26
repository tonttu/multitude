/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#if !defined (RADIANT_ARENA_ALLOCATOR_HPP)
#define RADIANT_ARENA_ALLOCATOR_HPP

#if 0
namespace Radiant
{
  template <size_t PoolSize>
  class MemoryPool
  {
  public:
    MemoryPool()
      : m_freeBytes(PoolSize)
      , m_usedBytes(0)
    {
      m_freeChunks.next = m_pool.data();
      m_freeChunks.size = 0;

      // Create the first chunk
      m_freeChunks.next->next = nullptr;
      m_freeChunks.next->size = PoolSize;
    }

    void * allocate(size_t bytes)
    {
      // Zero bytes requested
      if (bytes == 0)
        return nullptr;

      // The total size of the chunk required
      size_t neededBytes = bytes + sizeof(ChunkHeader);

      // Not enough space in this pool
      if (freeBytes() < neededBytes)
        return nullptr;

      // Find best fitting chunk
      size_t minSize = PoolSize;  // Set to max
      Chunk * minimumChunk = nullptr;
      for (auto it = m_freeChunks; it->next != nullptr; ) {
        size_t size = it->next->size;
        if (size >= neededBytes && size < minSize) {
          minSize = size;
          minimumChunk = it;
          if (size == neededBytes) // Perfect fit, we're done here
            break;
        }

        // Next chunk
        it = it->next;
      }

      // Sorry, couldn't find a chunk that fits
      if (minimumChunk == nullptr)
        return nullptr;

      // Found a usable chunk

      // Temporary copy of old chunk
      Chunk * oldNext = minimumChunk->next->next;
      size_t oldSize = minimumChunk->next->size;

      // Create the data chunk
      ChunkHeader * header = reinterpret_cast<ChunkHeader*>(minimumChunk->next);
      header->pool = this;
      void * ptr = reinterpret_cast<void *>(header + 1);
      
      if (oldSize > neededBytes + std::min(neededBytes * 2, 128)) {                 // Chunk is large enough: split it up
        // Scale what's left of the free chunk
        minimumChunk->next = (char *)(minimumChunk->next) + neededBytes;
        minimumChunk->next->next = oldNext;
        minimumChunk->next->size = oldSize - neededBytes;
      }
      else {
        // It's a small chunk, just take the whole thing
        minimumChunk->next = oldNext;
      }

      return ptr;
    }

    void deallocate(void * ptr)
    {

    }

    size_t size() const { return PoolSize; }
    size_t freeBytes() const { return m_freeBytes; }
    size_t usedBytes() const { return m_usedBytes; }

  private:
    size_t m_freeBytes;
    size_t m_usedBytes;

    struct ChunkHeader {
      MemoryPool<PoolSize> * pool;  // For quick pool-lookup during de-allocation
    };

    struct Chunk {
      Chunk * next;                 // Link to the previous chunk
      size_t size;                  // Data size of this chunk
    };

    Chunk m_freeChunks;

    std::array<unsigned char, PoolSize> m_pool;    // The actual data pool
  };

  template <size_t PoolSize>
  class MemoryArena
  {
  public:
    MemoryArena();
    ~MemoryArena();

    void * allocate(size_t bytes)
    {
      // Can't allocate something bigger than the poolsize
      assert (bytes < PoolSize);

      // Check for space in existing pools
      void * ptr = nullptr;
      for (auto & pool : m_pools) {
        ptr = pool.allocate(bytes);
        if (ptr != nullptr)
          return ptr;
      }

      // No pool with enough space: create a new pool
      m_pools.emplace_back();
      ptr = m_pools.back().allocate(bytes);
      // Newly created pool so allocation should usually work
      assert(ptr != nullptr);
      return ptr;
    }

    void deallocate(void * ptr)
    {
      /// @todo can keep track of correct pool in chunk-header
      for (auto & pool : m_pools)
        pool.deallocate(ptr);
    }

    size_t size() const
    {
      return m_pools.size() * PoolSize;
    }

  private:
    std::list<MemoryPool<PoolSize>> m_pools;
  };
}
#endif
#endif // RADIANT_ARENA_ALLOCATOR_HPP
