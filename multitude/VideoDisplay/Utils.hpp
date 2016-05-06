#ifndef UTILS_HPP
#define UTILS_HPP

#include <QAtomicInt>

#include <array>
#include <cassert>

/// Some utils for decoder implementations
namespace VideoDisplay
{
namespace Utils
{

  template <typename T, size_t N>
  class LockFreeQueue
  {
  public:
    LockFreeQueue();

    bool setSize(int items);
    int size() const;

    T * takeFree();
    void put();

    int itemCount() const;

    T * readyItem(int index = 0);
    T * lastReadyItem();
    void next();

  private:
    std::array<T, N> m_data;
    QAtomicInt m_readyItems;
    // index of the current queue head, "next ready item" (if m_readyItems > 0)
    int m_reader;
    // index of the next free item (if m_readyItems < m_size)
    int m_writer;
    int m_size;
  };

  // --------------------------------------------------------------------------

  template <typename T, std::size_t N>
  class MemoryPool
  {
  public:
    T * get();
    void put(const T & buffer);

  private:
    struct PoolItem
    {
      PoolItem() : inUse(false), data() {}
      bool inUse;
      T data;
    };

    std::array<PoolItem, N> m_data;
  };

  // --------------------------------------------------------------------------

  template <typename T, std::size_t N>
  T * MemoryPool<T, N>::get()
  {
    for(int i = 0; i < (int) N; ++i) {
      auto & item = m_data[i];
      if(item.inUse) continue;
      item.inUse = true;
      return &item.data;
    }
    return 0;
  }

  template <typename T, std::size_t N>
  void MemoryPool<T, N>::put(const T & t)
  {
    for(int i = 0; i < (int) N; ++i) {
      auto & item = m_data[i];
      if(item.inUse && &item.data == &t) {
        item.inUse = false;
        return;
      }
    }
    assert(false);
  }

  // --------------------------------------------------------------------------

  template <typename T, size_t N>
  LockFreeQueue<T, N>::LockFreeQueue()
    : m_reader(0)
    , m_writer(0)
    , m_size(N)
  {}

  template <typename T, size_t N>
  bool LockFreeQueue<T, N>::setSize(int items)
  {
    m_size = std::min(items, (int)m_data.size());
    return m_size == items;
  }

  template <typename T, size_t N>
  int LockFreeQueue<T, N>::size() const
  {
    return m_size;
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::takeFree()
  {
    if(m_readyItems >= m_size)
      return 0;

    int index = m_writer++;

    return & m_data[index % N];
  }

  template <typename T, size_t N>
  void LockFreeQueue<T, N>::put()
  {
    m_readyItems.ref();
  }

  template <typename T, size_t N>
  int LockFreeQueue<T, N>::itemCount() const
  {
    return m_readyItems;
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::readyItem(int index)
  {
    if(index >= m_readyItems) return nullptr;
    return & m_data[(m_reader + index) % N];
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::lastReadyItem()
  {
    if(m_readyItems < 1) return nullptr;
    return & m_data[(m_reader + m_readyItems - 1) % N];
  }

  template <typename T, size_t N>
  void LockFreeQueue<T, N>::next()
  {
    m_readyItems.deref();
    ++m_reader;
  }
}
}



#endif // UTILS_HPP
