#ifndef VIDEODISPLAY_MEMORYPOOL_H
#define VIDEODISPLAY_MEMORYPOOL_H

#include <array>
#include <cassert>

namespace VideoPlayer2
{
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
}

#endif // VIDEODISPLAY_MEMORYPOOL_H
