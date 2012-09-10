#if !defined (RADIANT_ALLOCATORS_HPP)
#define RADIANT_ALLOCATORS_HPP

#include <Radiant/Memory.hpp>

#include <cstddef>
#include <cstring>

#ifdef _MSC_VER
#pragma warning(disable: 4100)  // destroy() gives a false positive in VS2010
#endif

namespace Radiant
{
  /// Aligned memory allocator that can be used for STL containers
  /// @note Alignment should be a power of 2
  /// @todo static assert(!(alignment & (alignment-1)), "Alignment must be a power of 2")
  template<typename T, unsigned int alignment>
  class aligned_allocator
  {
    /// @cond
  public:
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;
    typedef T *       pointer;
    typedef const T * const_pointer;
    typedef T &       reference;
    typedef const T & const_reference;
    typedef T        value_type;

    template<typename U>
    struct rebind { typedef aligned_allocator<U, alignment> other; };

    aligned_allocator() throw() { }
    aligned_allocator(const aligned_allocator&) throw() { }

    template<typename U>
    aligned_allocator(const aligned_allocator<U,alignment> &) throw() { }

    ~aligned_allocator() throw() { }

    pointer address(reference ref) const { return Radiant::addressOf(ref); }
    const_pointer address(const_reference ref) const { return Radiant::addressOf(ref); }

    pointer allocate(size_type n, const void* = 0) { return reinterpret_cast<pointer>(Radiant::alignedMalloc(n * sizeof(T), alignment)); }
    void deallocate(pointer ptr, size_type) { Radiant::alignedFree(ptr); }

    /// @todo this should actually be defined as template<class U, class... Args> void construct(U* p, Args&&... args);
    void construct(pointer ptr) { ::new((void *)ptr) T(); }
    void construct(pointer ptr, const T & val) { ::new((void *)ptr) T(val); }
    void destroy(pointer ptr) { ptr->~T(); }

    size_t max_size() const { return (size_t)(-1) / sizeof(T); }

    /// @endcond
  };

#ifdef _MSC_VER
#pragma warning(default: 4100)
#endif

  /// @cond

  // Allocators are always equal
  template<typename T, unsigned int alignment>
  inline bool operator==(const aligned_allocator<T,alignment> &, const aligned_allocator<T,alignment> &) { return true; }

  template<typename T, unsigned int alignment>
  inline bool operator!=(const aligned_allocator<T,alignment> &, const aligned_allocator<T,alignment> &) { return false; }

  /// @endcond

  template <typename T, size_t BlockCount>
  class block_allocator
  {
  public:
    static_assert(BlockCount <= 0xFFFF, "Maximum of 65k blocks");

    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;
    typedef T *       pointer;
    typedef const T * const_pointer;
    typedef T &       reference;
    typedef const T & const_reference;
    typedef T        value_type;

    template<typename U>
    struct rebind { typedef block_allocator<U, BlockCount> other; };

    block_allocator() throw()
      : m_topChunk(new chunk())
    {
      m_activeChunk = m_topChunk.get();
    }

    block_allocator(const block_allocator&&) throw()
    {
      assert(false);
    }

    template<typename U>
    block_allocator(const block_allocator<U,BlockCount> && rhs) throw()
    {
      assert(false);
    }

    block_allocator(const block_allocator& rhs) throw()
    {
      assert(false);
    }

    template<typename U>
    block_allocator(const block_allocator<U,BlockCount> & rhs) throw()
    {
      assert(false);
    }

  public:
    ~block_allocator() throw() { }

    pointer address(reference ref) const { return Radiant::addressOf(ref); }
    const_pointer address(const_reference ref) const { return Radiant::addressOf(ref); }

    pointer allocate(size_type n, const void* = 0) {
      /// We're a block allocator, we only allocate per block
      assert(n == 1);

      void * ptr = m_activeChunk->tryAllocate();
      if (ptr == nullptr) {
        // Couldn't allocate with this pool: try the others
        m_activeChunk = m_topChunk.get();
        while (ptr == nullptr) {
          ptr = m_activeChunk->tryAllocate();

          if (ptr != nullptr) {
            // Found one
            break;
          }

          if (m_activeChunk->m_nextChunk == nullptr) {
            // No next chunk, allocate one
            m_activeChunk->m_nextChunk.reset(new chunk());
          }
          m_activeChunk = m_activeChunk->m_nextChunk.get();
        }
      }

      return reinterpret_cast<pointer>( ptr );
    }

    void deallocate(pointer ptr, size_type n)
    {
      /// We're a block allocator, we only deallocate per block
      assert(n == 1);
      chunk * c = m_topChunk.get();
      while (c != nullptr) {
        if (c->tryDeallocate(ptr))
          break;
        c = c->m_nextChunk.get();
      }
    }

    /// @todo this should actually be defined as template<class U, class... Args> void construct(U* p, Args&&... args);
    void construct(pointer ptr) { ::new((void *)ptr) T(); }
    void construct(pointer ptr, const T & val) { ::new((void *)ptr) T(val); }
    void destroy(pointer ptr) { ptr->~T(); }

    size_t max_size() const { return (size_t)(-1) / sizeof(T); }

  public:

    friend bool operator==(const block_allocator & lhs, const block_allocator & rhs);
    friend bool operator!=(const block_allocator & lhs, const block_allocator & rhs);

    class chunk
    {
    public:
      chunk()
        : m_freeCount(0)
      {
        memset(m_freeBits, 0, sizeof(m_freeBits));

#if defined (RADIANT_DEBUG)
        // Fill with debug pattern
        memset(m_data, 0xCC, sizeof(T));
#endif
      }

      void * tryAllocate()
      {
        if (m_freeCount > 0)
          return allocate(m_freeList[--m_freeCount]);

        return nullptr;
      }

      bool tryDeallocate(void * ptr)
      {
        /// Test if we're in range
        if (m_data <= ptr && ptr < m_data + poolsize_bytes) {
          unsigned int index = (unsigned int)(((char *)ptr - m_data)) / sizeof(T);
          unsigned int bit_index = index / 32;
          unsigned int bit = 1 << (index & 31);
#if defined (RADIANT_DEBUG)
          unsigned int bitcheck = m_freeBits[bit_index];
          assert((bitcheck & bit) == bit);
#endif
          // Clear bit for this block
          m_freeBits[bit_index] &= ~bit;
          // Add block to the freelist
          m_freeList[m_freeCount++] = index;

#if defined (RADIANT_DEBUG)
          // Fill with debug pattern
          memset(ptr, 0xCD, sizeof(T));
#endif
          return true;
        }
        return false;
      }

    public:

      void * allocate(unsigned int index) {
        unsigned int bit_index = index / 32;
        unsigned int bit = 1 << (index & 31);

#if defined (RADIANT_DEBUG)
        unsigned int bitcheck = m_freeBits[bit_index];
        assert((bitcheck & bit) == 0);
#endif
        m_freeBits[bit_index] |= bit;
        return m_data + index * sizeof(T);
      }

    public:
      friend class block_allocator;
      enum {
        poolsize_bytes = sizeof(T) * BlockCount,
        freebits_count = BlockCount / sizeof(unsigned int) + 1,
      };
      char m_data[poolsize_bytes];

      unsigned int m_freeCount;
      unsigned int m_freeBits[freebits_count];
      unsigned short m_freeList[BlockCount];
      std::unique_ptr<chunk> m_nextChunk;
    };

    std::unique_ptr<chunk> m_topChunk;
    chunk * m_activeChunk;
  };

  template<typename T, unsigned int BlockCount>
  inline bool operator==(const block_allocator<T,BlockCount> & lhs, const block_allocator<T,BlockCount> & rhs) { return lhs.m_topChunk == rhs.m_topChunk; }

  template<typename T, unsigned int BlockCount>
  inline bool operator!=(const block_allocator<T,BlockCount> & lhs, const block_allocator<T,BlockCount> & rhs) { return !(lhs == rhs); }

}

#endif // RADIANT_ALLOCATORS_HPP
