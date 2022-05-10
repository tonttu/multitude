#pragma once

#include <vector>
#include <memory>

namespace Radiant
{
  /// Memory allocator that allocates uninitialized arrays of type T from
  /// continuous memory. All allocated memory is invalidated with a call to
  /// clear(). The class attempts to allocate all memory from a single continous
  /// memory segment, but if that runs out, it will use additional overflow
  /// buffers. These overflow buffers are released in clear() and the main buffer
  /// size is respectively increased, so that in the future the class is able
  /// to allocate all memory from one continous block. Technically only
  /// releases memory in the destructor.
  ///
  /// This is useful for relatively small repeated allocations, for instance
  /// buffers required for rendering, that can be released after each frame.
  template <typename T>
  class VectorAllocator
  {
  public:
    /// @param reserve initially reserve memory for this many elements
    VectorAllocator(size_t reserve);
    /// Release all memory
    ~VectorAllocator() = default;

    /// Allocate elements number of T elements from a continous memory segment
    T * allocate(size_t elements);
    /// Invalidate all previously allocated memory segments, free overflow memory,
    /// optimize main memory buffer for future allocations
    void clear();

  private:
    struct Vector
    {
      std::unique_ptr<T[]> data;
      size_t size = 0;
      size_t capacity = 0;
    };
    Vector m_data;
    std::vector<Vector> m_overflow;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename T>
  VectorAllocator<T>::VectorAllocator(size_t reserve)
  {
    if (reserve > 0) {
      m_data.capacity = reserve;
      m_data.data.reset(new T[reserve]);
    }
  }

  template<typename T>
  T * VectorAllocator<T>::allocate(size_t elements)
  {
    if (elements == 0)
      return nullptr;

    size_t available = m_data.capacity - m_data.size;
    if (available >= elements) {
      T * data = m_data.data.get() + m_data.size;
      m_data.size += elements;
      return data;
    }

    if (m_data.size == 0) {
      m_data.data.reset(new T[elements]);
      m_data.capacity = elements;
      m_data.size = elements;
      return m_data.data.get();
    }

    size_t reserve = m_data.capacity * 2;
    for (Vector & overflow: m_overflow) {
      available = overflow.capacity - overflow.size;
      if (available >= elements) {
        T * data = overflow.data.get() + overflow.size;
        overflow.size += elements;
        return data;
      }
      reserve = overflow.capacity * 2;
    }

    m_overflow.emplace_back();
    Vector & v = m_overflow.back();
    v.capacity = std::max(reserve, elements);
    v.size = elements;
    v.data.reset(new T[v.capacity]);
    return v.data.get();
  }

  template<typename T>
  void VectorAllocator<T>::clear()
  {
    if (!m_overflow.empty()) {
      size_t totalSize = m_data.size;
      for (const Vector & overflow: m_overflow)
        totalSize += overflow.size;
      m_overflow.clear();
      m_data.capacity = totalSize;
      m_data.data.reset(new T[totalSize]);
    }
    m_data.size = 0;
  }
}
