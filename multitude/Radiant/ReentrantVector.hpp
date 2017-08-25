#ifndef RADIANT_REENTRANTVECTOR_HPP
#define RADIANT_REENTRANTVECTOR_HPP

#include <vector>
#include <cstddef>
#include <cassert>
#include <algorithm>

namespace Radiant
{
  /// Vector that can be modified while iterating it. Iterators for this vector
  /// don't get invalidated even if elements are added to the vector or removed
  /// from the vector, including the element where the iterator currently points to.
  ///
  /// This class is not thread-safe, it is designed to be used from one thread only.
  ///
  /// This class is perfect for storing callbacks or objects with virtual functions
  /// that you need to iterate and call. Even if those functions would modify the
  /// container or even remove itself from the container, the iterator will still
  /// be valid.
  ///
  /// For example:
  /// @code
  /// ReentrantVector<std::function<void()>> & callbacks = ...;
  /// for (auto & func: callbacks)
  ///   func(); // function might modify callbacks, even remove itself from it
  /// @endcode
  template <typename T, typename Allocator = typename std::vector<T>::allocator_type>
  class ReentrantVector : private std::vector<T, Allocator>
  {
  public:
    /// Non-copyable limited random access iterator to ReentrantVector
    class ConstIterator
    {
    public:
      typedef typename ReentrantVector<T, Allocator>::value_type value_type;
      typedef typename ReentrantVector<T, Allocator>::difference_type difference_type;
      typedef typename ReentrantVector<T, Allocator>::reference reference;
      typedef typename ReentrantVector<T, Allocator>::pointer pointer;

    public:
      /// Construct a valid iterator. Use ReentrantVector::begin and end instead of this.
      inline ConstIterator(const ReentrantVector<T, Allocator> * container, size_t idx);

      /// Construct a null iterator
      inline ConstIterator() noexcept {}

      /// Destruct the iterator, remove it from the ReentrantVector list of iterators
      inline ~ConstIterator();

      /// Move operators
      inline ConstIterator(ConstIterator && it);
      inline ConstIterator & operator=(ConstIterator && it);

      /// Random access iterator operations
      inline ConstIterator & operator++() noexcept;
      inline ConstIterator & operator--() noexcept;
      inline ConstIterator & operator+=(difference_type n) noexcept;
      inline ConstIterator & operator-=(difference_type n) noexcept;

      /// Compares iterator locations. Operators need to point to the same vector
      inline bool operator==(ConstIterator & other) const noexcept;
      inline bool operator!=(ConstIterator & other) const noexcept;

      /// Returns the index of the element this iterators points to inside the vector
      inline size_t index() const noexcept;

      /// Returns the element where the iterator points to
      inline const T & operator*() const noexcept;
      inline const T * operator->() const noexcept;

      /// Returns true if the item where the iterator points to was deleted
      /// after the iterator was last changed
      inline bool wasCurrentItemDeleted() const noexcept;

    private:
      /// This iterator is not copyable
      ConstIterator(const ConstIterator &) = delete;
      ConstIterator & operator=(const ConstIterator &) = delete;

    protected:
      friend class ReentrantVector<T, Allocator>;

      const ReentrantVector<T, Allocator> * m_vector = nullptr;
      size_t m_idx = 0;
      bool m_currentItemDeleted = false;
    };

    /// Mutable version of the iterator. To keep the implementation simple, this
    /// shares the implementation with const iterator.
    class Iterator : public ConstIterator
    {
    public:
      using ConstIterator::ConstIterator;

      /// Returns mutable reference to the object the iterator points to
      inline T & operator*() noexcept;
      inline T * operator->() noexcept;
    };

  public:
    /// Types
    typedef std::vector<T, Allocator> Base;
    typedef Iterator iterator;
    typedef ConstIterator const_iterator;
    using typename Base::reference;
    using typename Base::const_reference;
    using typename Base::size_type;
    using typename Base::difference_type;
    using typename Base::value_type;
    using typename Base::allocator_type;
    using typename Base::pointer;
    using typename Base::const_pointer;

    /// Construct/copy/destroy
    using Base::vector;
    using Base::get_allocator;
    ~ReentrantVector();

    /// Iterators
    inline Iterator begin() noexcept;
    inline Iterator end() noexcept;
    inline ConstIterator begin() const noexcept;
    inline ConstIterator end() const noexcept;
    inline ConstIterator cbegin() const noexcept;
    inline ConstIterator cend() const noexcept;

    /// Capacity
    using Base::size;
    using Base::max_size;
    using Base::capacity;
    using Base::empty;
    using Base::reserve;
    using Base::shrink_to_fit;

    /// Element access
    using Base::operator[];
    using Base::at;
    using Base::front;
    using Base::back;

    /// Data access
    using Base::data;

    /// Modifiers
    /// push_back and insert use universal references
    template <typename Y>
    void push_back(Y && value);
    inline void pop_back();
    template <typename Y>

    /// Insert and erase use element index instead of iterators, since iterators
    /// are more expensive in this implementation. They also don't return
    /// iterators.
    void insert(size_t idx, Y && value);
    void erase(size_t idx);
    inline void swap(ReentrantVector<T, Allocator> & other);
    inline void clear() noexcept;

    /// Access to the underlying container. It is not safe to iterate this and
    /// modify the vector at the same time.
    const std::vector<T, Allocator> & unsafeVector() const;

  private:
    /// Private functions called by Iterators
    void addIterator(ConstIterator * it) const;
    void removeIterator(ConstIterator * it) const;
    void replaceIterator(ConstIterator * from, ConstIterator * to) const;

  private:
    /// Modified in add/remove/replaceIterator, must be const.
    mutable std::vector<ConstIterator*> m_iterators;
  };


  ///////////////////////////////////////////////////////////////////////////////
  /// ConstIterator Implementation

  template <typename T, typename Allocator>
  ReentrantVector<T, Allocator>::ConstIterator::ConstIterator(const ReentrantVector<T, Allocator> * container, size_t idx)
    : m_vector(container)
    , m_idx(idx)
  {
    m_vector->addIterator(this);
  }

  template <typename T, typename Allocator>
  ReentrantVector<T, Allocator>::ConstIterator::~ConstIterator()
  {
    if (m_vector)
      m_vector->removeIterator(this);
  }

  template <typename T, typename Allocator>
  ReentrantVector<T, Allocator>::ConstIterator::ConstIterator(ConstIterator && it)
    : m_vector(it.m_vector)
    , m_idx(it.m_idx)
    , m_currentItemDeleted(it.m_currentItemDeleted)
  {
    if (m_vector) {
      m_vector->replaceIterator(&it, this);
    }
    it.m_vector = nullptr;
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator & ReentrantVector<T, Allocator>::ConstIterator::operator=(ConstIterator && it)
  {
    if (m_vector && it.m_vector) {
      if (m_vector != it.m_vector) {
        m_vector->removeIterator(this);
        it.m_vector->addIterator(this);
      }
      it.m_vector->removeIterator(&it);
    } else if (m_vector) {
      m_vector->removeIterator(this);
    } else if (it.m_vector) {
      it.m_vector->replaceIterator(&it, this);
    }

    m_vector = it.m_vector;
    it.m_vector = nullptr;
    m_idx = it.m_idx;
    m_currentItemDeleted = it.m_currentItemDeleted;

    return *this;
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator & ReentrantVector<T, Allocator>::ConstIterator::operator++() noexcept
  {
    if (m_currentItemDeleted) {
      m_currentItemDeleted = false;
    } else {
      ++m_idx;
    }
    return *this;
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator & ReentrantVector<T, Allocator>::ConstIterator::operator--() noexcept
  {
    m_currentItemDeleted = false;
    --m_idx;
    return *this;
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator & ReentrantVector<T, Allocator>::ConstIterator::operator+=(difference_type n) noexcept
  {
    if (n > 0 && m_currentItemDeleted) {
      m_currentItemDeleted = false;
      m_idx += n - 1;
    } else {
      m_idx += n;
    }
    return *this;
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator & ReentrantVector<T, Allocator>::ConstIterator::operator-=(difference_type n) noexcept
  {
    return operator+=(-n);
  }

  template <typename T, typename Allocator>
  bool ReentrantVector<T, Allocator>::ConstIterator::operator==(ConstIterator & other) const noexcept
  {
    assert(m_vector == other.m_vector);
    return m_idx == other.m_idx;
  }

  template <typename T, typename Allocator>
  bool ReentrantVector<T, Allocator>::ConstIterator::operator!=(ConstIterator & other) const noexcept
  {
    assert(m_vector == other.m_vector);
    return m_idx != other.m_idx;
  }

  template <typename T, typename Allocator>
  size_t ReentrantVector<T, Allocator>::ConstIterator::index() const noexcept
  {
    return m_idx;
  }

  template <typename T, typename Allocator>
  const T & ReentrantVector<T, Allocator>::ConstIterator::operator*() const noexcept
  {
    assert(!m_currentItemDeleted);
    return (*m_vector)[m_idx];
  }

  template <typename T, typename Allocator>
  const T * ReentrantVector<T, Allocator>::ConstIterator::operator->() const noexcept
  {
    assert(!m_currentItemDeleted);
    return &(*m_vector)[m_idx];
  }

  template <typename T, typename Allocator>
  bool ReentrantVector<T, Allocator>::ConstIterator::wasCurrentItemDeleted() const noexcept
  {
    return m_currentItemDeleted;
  }


  ///////////////////////////////////////////////////////////////////////////////
  /// Iterator Implementation

  template <typename T, typename Allocator>
  T & ReentrantVector<T, Allocator>::Iterator::operator*() noexcept
  {
    assert(!this->m_currentItemDeleted);
    return const_cast<ReentrantVector<T, Allocator>&>(*this->m_vector)[this->m_idx];
  }

  template <typename T, typename Allocator>
  T * ReentrantVector<T, Allocator>::Iterator::operator->() noexcept
  {
    assert(!this->m_currentItemDeleted);
    return &const_cast<ReentrantVector<T, Allocator>&>(*this->m_vector)[this->m_idx];
  }


  ///////////////////////////////////////////////////////////////////////////////
  /// ReentrantVector Implementation

  template <typename T, typename Allocator>
  ReentrantVector<T, Allocator>::~ReentrantVector()
  {
    for (ConstIterator * it: m_iterators)
      it->m_vector = nullptr;
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::Iterator ReentrantVector<T, Allocator>::begin() noexcept
  {
    return Iterator(this, 0);
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::Iterator ReentrantVector<T, Allocator>::end() noexcept
  {
    return Iterator(this, size());
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator ReentrantVector<T, Allocator>::begin() const noexcept
  {
    return ConstIterator(this, 0);
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator ReentrantVector<T, Allocator>::end() const noexcept
  {
    return ConstIterator(this, size());
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator ReentrantVector<T, Allocator>::cbegin() const noexcept
  {
    return ConstIterator(this, 0);
  }

  template <typename T, typename Allocator>
  typename ReentrantVector<T, Allocator>::ConstIterator ReentrantVector<T, Allocator>::cend() const noexcept
  {
    return ConstIterator(this, size());
  }

  template <typename T, typename Allocator>
  template <typename Y>
  void ReentrantVector<T, Allocator>::push_back(Y && value)
  {
    insert(size(), std::forward<Y>(value));
  }

  template <typename T, typename Allocator>
  void ReentrantVector<T, Allocator>::pop_back()
  {
    erase(size() - 1);
  }

  template <typename T, typename Allocator>
  template <typename Y>
  void ReentrantVector<T, Allocator>::insert(size_t idx, Y && value)
  {
    Base::insert(Base::begin() + idx, std::forward<Y>(value));

    for (ConstIterator * it: m_iterators)
      if (idx <= it->m_idx)
        ++it->m_idx;
  }

  template <typename T, typename Allocator>
  void ReentrantVector<T, Allocator>::erase(size_t idx)
  {
    Base::erase(Base::begin() + idx);

    for (ConstIterator * it: m_iterators) {
      if (idx < it->m_idx) {
        --it->m_idx;
      } else if (idx == it->m_idx) {
        it->m_currentItemDeleted = true;
      }
    }
  }

  template <typename T, typename Allocator>
  void ReentrantVector<T, Allocator>::swap(ReentrantVector<T, Allocator> & other)
  {
    Base::swap(other);
    m_iterators.swap(other.m_iterators);
  }

  template <typename T, typename Allocator>
  void ReentrantVector<T, Allocator>::clear() noexcept
  {
    size_t idx = size();
    Base::clear();

    for (ConstIterator * it: m_iterators) {
      if (idx == it->m_idx) {
        it->m_idx = 0;
      } else if (idx > it->m_idx) {
        it->m_idx = 0;
        it->m_currentItemDeleted = true;
      }
    }
  }

  template <typename T, typename Allocator>
  const std::vector<T, Allocator> & ReentrantVector<T, Allocator>::unsafeVector() const
  {
    return *this;
  }

  template <typename T, typename Allocator>
  void ReentrantVector<T, Allocator>::addIterator(ConstIterator * it) const
  {
    m_iterators.push_back(it);
  }

  template <typename T, typename Allocator>
  void ReentrantVector<T, Allocator>::removeIterator(ConstIterator * it) const
  {
    m_iterators.erase(std::find(m_iterators.begin(), m_iterators.end(), it));
  }

  template <typename T, typename Allocator>
  void ReentrantVector<T, Allocator>::replaceIterator(ConstIterator * from, ConstIterator * to) const
  {
    *std::find(m_iterators.begin(), m_iterators.end(), from) = to;
  }

} // namespace Radiant

#endif // REENTRANTVECTOR_HPP
