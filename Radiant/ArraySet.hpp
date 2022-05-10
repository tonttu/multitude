#ifndef RADIANT_ARRAY_SET_HPP
#define RADIANT_ARRAY_SET_HPP

#include <vector>

#include <QByteArray>
#include <QList>

namespace Radiant
{
  template <typename T>
  inline int compare(const T & a, const T & b)
  {
    return (b < a) - (a < b);
  }

  template <>
  inline int compare<QByteArray>(const QByteArray & a, const QByteArray & b)
  {
    return qstrcmp(a, b);
  }

  /// Set implementation that uses simple std::vector to store the elements.
  ///
  /// This can be used instead of std::set when number is elements is small.
  /// Do not ever use this class unless you have profiled that it actually
  /// makes a difference!
  ///
  /// Notice that all non-const operations might invalidate all iterators.
  template <typename Key, typename Allocator = typename std::vector<Key>::allocator_type>
  class ArraySet
  {
  public:

    // Typedefs

    typedef typename std::vector<Key, Allocator> container;

    typedef Key key_type;
    typedef Key value_type;
    typedef typename container::size_type size_type;
    typedef typename container::difference_type difference_type;
    typedef Allocator allocator_type;
    typedef value_type & reference;
    typedef value_type & const_reference;
    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    typedef typename container::iterator iterator;
    typedef typename container::const_iterator const_iterator;
    typedef typename container::reverse_iterator reverse_iterator;
    typedef typename container::const_reverse_iterator const_reverse_iterator;


    // Constructors

    ArraySet() {}
    ArraySet(int reserve) { m_data.reserve(reserve); }
    ArraySet(std::size_t reserve) { m_data.reserve(reserve); }

    ArraySet(const ArraySet & set) : m_data(set.m_data) {}
    ArraySet(ArraySet && set) : m_data(std::move(set.m_data)) {}

    template <typename it>
    ArraySet(it first, it last);

    template <typename T>
    ArraySet(const T & list);


    // Operators

    ArraySet & operator=(const ArraySet & set) { m_data = set.m_data; return *this; }
    ArraySet & operator=(ArraySet && set) { m_data = std::move(set.m_data); return *this; }

    template <typename T>
    void operator=(const T & list);

    ArraySet operator-(const ArraySet & set) const;

    bool operator==(const ArraySet & set) const { return m_data == set.m_data; }

    void operator<<(const Key & key) { insert(key); }


    // Iterators

    iterator begin() { return m_data.begin(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator cbegin() const { return m_data.cbegin(); }

    iterator end() { return m_data.end(); }
    const_iterator end() const { return m_data.end(); }
    const_iterator cend() const { return m_data.cend(); }

    reverse_iterator rbegin() { return m_data.rbegin(); }
    const_reverse_iterator rbegin() const { return m_data.rbegin(); }
    const_reverse_iterator crbegin() const { return m_data.crbegin(); }

    reverse_iterator rend() { return m_data.rend(); }
    const_reverse_iterator rend() const { return m_data.rend(); }
    const_reverse_iterator crend() const { return m_data.crend(); }


    // Capacity

    bool empty() const { return m_data.empty(); }
    bool isEmpty() const { return m_data.empty(); }

    std::size_t size() const { return m_data.size(); }


    // Modifiers

    void clear() { m_data.clear(); }

    bool insert(const Key & value);
    bool insert(Key && value);

    template <typename it>
    void insert(it first, it last);

    iterator erase(iterator pos) { return m_data.erase(pos); }
    bool erase(const Key & key);

    void swap(ArraySet & set) { std::swap(m_data, set.m_data); }


    // Lookup

    std::size_t count(const Key & key) const;
    
    iterator find(const Key & key);
    const_iterator find(const Key & key) const;

    int compare(const ArraySet & set) const;

    bool contains(const Key & key) const;
    bool containsAll(const ArraySet & set) const;

    QList<Key> toList() const;

  private:
    container m_data;
  };

  template <typename Key, typename Allocator>
  template <typename it>
  ArraySet<Key, Allocator>::ArraySet(it first, it last)
  {
    while (first != last)
      m_data.push_back(*first++);
    std::sort(m_data.begin(), m_data.end());
  }

  template <typename Key, typename Allocator>
  template <typename T>
  ArraySet<Key, Allocator>::ArraySet(const T & list)
  {
    m_data.reserve(list.size());
    for (const Key & key: list)
      m_data.push_back(key);
    std::sort(m_data.begin(), m_data.end());
  }

  template <typename Key, typename Allocator>
  template <typename T>
  void ArraySet<Key, Allocator>::operator=(const T & list)
  {
    m_data.resize(0);
    m_data.reserve(list.size());
    for (const Key & key: list)
      m_data.push_back(key);
    std::sort(m_data.begin(), m_data.end());
  }

  template <typename Key, typename Allocator>
  ArraySet<Key, Allocator> ArraySet<Key, Allocator>::operator-(const ArraySet<Key, Allocator> & set) const
  {
    using Radiant::compare;
    ArraySet<Key, Allocator> diff(m_data.size());
    for (std::size_t a = 0, b = 0, ae = m_data.size(), be = set.m_data.size(); a < ae;) {
      if (b == be) {
        do {
          diff.m_data.push_back(m_data[a++]);
        } while (a < ae);
        return diff;
      }
      const int c = compare<Key>(m_data[a], set.m_data[b]);
      if (c < 0)
        diff.m_data.push_back(m_data[a++]);
      else if (c == 0)
        ++a, ++b;
      else
        ++b;
    }
    return diff;
  }

  template <typename Key, typename Allocator>
  bool ArraySet<Key, Allocator>::insert(const Key & value)
  {
    using Radiant::compare;
    for (std::size_t i = 0, m = m_data.size(); i < m; ++i) {
      const int c = compare<Key>(m_data[i], value);
      if (c > 0) {
        m_data.push_back(m_data.back());
        for (std::size_t j = m - 1; j > i; --j)
          m_data[j] = std::move(m_data[j-1]);
        m_data[i] = value;
        return true;
      }
      if (c == 0)
        return false;
    }
    m_data.push_back(value);
    return true;
  }

  template <typename Key, typename Allocator>
  bool ArraySet<Key, Allocator>::insert(Key && value)
  {
    using Radiant::compare;
    for (std::size_t i = 0, m = m_data.size(); i < m; ++i) {
      const int c = compare<Key>(m_data[i], value);
      if (c > 0) {
        m_data.push_back(m_data.back());
        for (std::size_t j = m - 1; j > i; --j)
          m_data[j] = std::move(m_data[j-1]);
        m_data[i] = std::move(value);
        return true;
      }
      if (c == 0)
        return false;
    }
    m_data.push_back(std::move(value));
    return true;
  }

  template <typename Key, typename Allocator>
  template <typename it>
  void ArraySet<Key, Allocator>::insert(it first, it last)
  {
    while (first != last)
      insert(*first++);
  }

  template <typename Key, typename Allocator>
  bool ArraySet<Key, Allocator>::erase(const Key & key)
  {
    for (std::size_t i = 0, m = m_data.size(); i < m; ++i) {
      if (m_data[i] == key) {
        for (--m; i < m; ++i)
          m_data[i] = std::move(m_data[i+1]);
        m_data.resize(m_data.size()-1);
        return true;
      }
    }
    return false;
  }

  template <typename Key, typename Allocator>
  std::size_t ArraySet<Key, Allocator>::count(const Key & key) const
  {
    for (auto & value: m_data)
      if (value == key)
        return 1;
    return 0;
  }
 
  template <typename Key, typename Allocator>
  typename ArraySet<Key, Allocator>::iterator ArraySet<Key, Allocator>::find(const Key & key)
  {
    return std::find(m_data.begin(), m_data.end(), key);
  }

  template <typename Key, typename Allocator>
  typename ArraySet<Key, Allocator>::const_iterator ArraySet<Key, Allocator>::find(const Key & key) const
  {
    return std::find(m_data.begin(), m_data.end(), key);
  }

  template <typename Key, typename Allocator>
  int ArraySet<Key, Allocator>::compare(const ArraySet & set) const
  {
    using Radiant::compare;
    const int l0 = m_data.size(), l1 = set.m_data.size();
    if (l0 != l1)
      return l0 < l1 ? -1 : 1;

    for (int i = 0; i < l0; ++i) {
      const int c = compare<Key>(m_data[i], set.m_data[i]);
      if (c)
        return c;
    }
    return 0;
  }

  template <typename Key, typename Allocator>
  bool ArraySet<Key, Allocator>::contains(const Key & key) const
  {
    for (const auto & k: m_data)
      if (key == k)
        return true;
    return false;
  }

  template <typename Key, typename Allocator>
  bool ArraySet<Key, Allocator>::containsAll(const ArraySet & set) const
  {
    using Radiant::compare;
    for (std::size_t a = 0, b = 0, ae = m_data.size(), be = set.m_data.size(); b < be; ) {
      if (a == ae)
        return false;

      const int c = compare<Key>(m_data[a], set.m_data[b]);
      if (c > 0)
        return false;
      else if (c < 0)
        ++a;
      else
        ++a, ++b;
    }

    return true;
  }

  template<typename Key, typename Allocator>
  QList<Key> ArraySet<Key, Allocator>::toList() const
  {
    QList<Key> result;

    for(auto & i : *this)
      result.append(i);

    return result;
  }

  template <typename Key, typename Allocator>
  inline void swap(ArraySet<Key, Allocator> & a, ArraySet<Key, Allocator> & b)
  {
    a.swap(b);
  }
}

#endif // RADIANT_ARRAY_SET_HPP
