#if !defined (RADIANT_ALLOCATORS_HPP)
#define RADIANT_ALLOCATORS_HPP

#include <Radiant/Memory.hpp>

#include <cstddef>

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
}

#endif // RADIANT_ALLOCATORS_HPP
