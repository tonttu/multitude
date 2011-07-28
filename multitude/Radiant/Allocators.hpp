#if !defined (RADIANT_ALLOCATORS_HPP)
#define RADIANT_ALLOCATORS_HPP

#include <Radiant/Memory.hpp>

namespace Radiant
{
  /// Aligned memory allocator that can be used for STL containers
  /// @note Alignment should be a power of 2
  /// @todo static assert(!(alignment & (alignment-1)), "Alignment must be a power of 2")
  template<typename T, unsigned int alignment>
  class aligned_allocator
  {
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

    void construct(pointer ptr, const T & val) { ::new((void *)ptr) T(val); }
    void destroy(pointer ptr) { ptr->~T(); }

    size_t max_size() const { return (size_t)(-1) / sizeof(T); }
  };

  // Allocators are always equal
  template<typename T, unsigned int alignment>
  inline bool operator==(const aligned_allocator<T,alignment> &, const aligned_allocator<T,alignment> &) { return true; }

  template<typename T, unsigned int alignment>
  inline bool operator!=(const aligned_allocator<T,alignment> &, const aligned_allocator<T,alignment> &) { return false; }
}

#endif // RADIANT_ALLOCATORS_HPP
