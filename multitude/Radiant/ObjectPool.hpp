#ifndef RADIANT_OBJECTPOOL_HPP
#define RADIANT_OBJECTPOOL_HPP

#include "Mutex.hpp"
#include "BGThread.hpp"

namespace Radiant
{
  class ObjectPool
  {
  public:
    RADIANT_API ObjectPool(std::size_t poolSize);
    RADIANT_API virtual ~ObjectPool();

    inline void setPoolSize(std::size_t size) { m_poolSize = size; }
    inline std::size_t poolSize() const { return m_poolSize; }

    /// Fill the pool with new objects
    /// @returns number of objects created
    virtual int fill() = 0;
    virtual void clear() = 0;

    RADIANT_API static std::pair<std::size_t, std::size_t> fillAll();
    RADIANT_API static Radiant::TaskPtr createFillTask();
    RADIANT_API static void setAllPoolSizes(std::size_t size);
    RADIANT_API static void clearAll();

  private:
    std::size_t m_poolSize;
  };

  /// Object pool for movable objects
  template <typename T>
  class ObjectPoolT : public ObjectPool
  {
  public:
    typedef std::function<T()> FactoryFunc;

  public:
    inline ObjectPoolT(std::size_t size, FactoryFunc factory);

    inline virtual int fill() OVERRIDE;
    inline virtual void clear() OVERRIDE;

    inline T get();

  private:
    Mutex m_objectsMutex;
    std::vector<T> m_objects;

    FactoryFunc m_factory;
  };

  template <typename T>
  ObjectPoolT<T>::ObjectPoolT(std::size_t size, FactoryFunc factory)
    : ObjectPool(size)
    , m_factory(factory)
  {
  }

  template <typename T>
  int ObjectPoolT<T>::fill()
  {
    std::size_t created = 0;
    std::size_t size;
    {
      Guard g(m_objectsMutex);
      size = m_objects.size();
      m_objects.reserve(poolSize());
    }

    while (size < poolSize()) {
      auto t = m_factory();
      ++created;

      Guard g(m_objectsMutex);
      m_objects.push_back(std::move(t));
      size = m_objects.size();
    }

    return created;
  }

  template <typename T>
  void ObjectPoolT<T>::clear()
  {
    std::vector<T> objects;
    {
      Guard g(m_objectsMutex);
      using std::swap;
      swap(m_objects, objects);
    }
  }

  template <typename T>
  T ObjectPoolT<T>::get()
  {
    Guard g(m_objectsMutex);
    if (m_objects.size() > 0) {
      auto t = std::move(m_objects.back());
      m_objects.pop_back();
      return t;
    } else {
      return T();
    }
  }

} // namespace Radiant

#endif // RADIANT_OBJECTPOOL_HPP
