/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_ARRAY_MAP_HPP
#define RADIANT_ARRAY_MAP_HPP

#include <vector>

namespace Radiant
{
  /// Non-ordered map implementation that uses simple std::vector to store the elements.
  ///
  /// This can be used instead of std::map when number is elements is small.
  /// Never use this class unless you have profiled that it actually makes
  /// a difference!
  ///
  /// Notice that all non-const operations might invalidate all iterators.
  template <typename Key, typename T, typename Allocator = typename std::vector<std::pair<Key, T>>::allocator_type>
  class ArrayMap
  {
  public:

    // Typedefs

    typedef typename std::vector<std::pair<Key, T>, Allocator> container;

    typedef Key key_type;
    typedef T mapped_type;
    // Compared to std::map, in this implementation Key is mutable, since we
    // will and can modify it in erase-function
    typedef std::pair<Key, T> value_type;
    typedef typename container::size_type size_type;
    typedef typename container::difference_type difference_type;
    typedef Allocator allocator_type;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef typename container::pointer pointer;
    typedef typename container::const_pointer const_pointer;
    typedef typename container::iterator iterator;
    typedef typename container::const_iterator const_iterator;
    typedef typename container::reverse_iterator reverse_iterator;
    typedef typename container::const_reverse_iterator const_reverse_iterator;


    // Constructors

    ArrayMap() {}
    ArrayMap(int reserve) { m_data.reserve(reserve); }
    ArrayMap(std::size_t reserve) { m_data.reserve(reserve); }

    ArrayMap(const ArrayMap & map) : m_data(map.m_data) {}
    ArrayMap(ArrayMap && map) : m_data(std::move(map.m_data)) {}

    template <typename it>
    ArrayMap(it first, it last);

    template <typename Y>
    ArrayMap(const Y & map);


    // Operators

    ArrayMap & operator=(const ArrayMap & map) { m_data = map.m_data; return *this; }
    ArrayMap & operator=(ArrayMap && map) { m_data = std::move(map.m_data); return *this; }

    template <typename Y>
    void operator=(const Y & map);

    bool operator==(const ArrayMap & map) const { return m_data == map.m_data; }
    bool operator!=(const ArrayMap & map) const { return m_data != map.m_data; }

    T & operator[](const Key & key);
    T & operator[](Key && key);


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

    void insert(const value_type & value);
    void insert(value_type && value);

    iterator erase(iterator it);
    std::size_t erase(const Key & key);

    void swap(ArrayMap & other) { m_data.swap(other); }


    // Lookup

    T value(const Key & key) const;

    bool contains(const Key & key) const;
    std::size_t count(const Key & key) const;

    iterator find(const Key & key);
    const_iterator find(const Key & key) const;

    /// @cond

    // Other
    const container & vector() const { return m_data; }
    container & vector() { return m_data; }

    /// @endcond

  private:
    container m_data;
  };
  template <typename Key, typename T, typename Allocator>
  template <typename it>
  ArrayMap<Key, T, Allocator>::ArrayMap(it first, it last)
  {
    while (first != last)
      m_data.push_back(*first++);
  }

  template <typename Key, typename T, typename Allocator>
  template <typename Y>
  ArrayMap<Key, T, Allocator>::ArrayMap(const Y & map)
  {
    m_data.reserve(map.size());
    for (auto & p: map)
      m_data.push_back(p);
  }

  template <typename Key, typename T, typename Allocator>
  template <typename Y>
  void ArrayMap<Key, T, Allocator>::operator=(const Y & map)
  {
    m_data.clear();
    m_data.reserve(map.size());
    for (auto & p: map)
      m_data.push_back(p);
  }

  template <typename Key, typename T, typename Allocator>
  T & ArrayMap<Key, T, Allocator>::operator[](const Key & key)
  {
    for (auto & p: m_data)
      if (p.first == key)
        return p.second;

    m_data.emplace_back(key, T());
    return m_data.back().second;
  }

  template <typename Key, typename T, typename Allocator>
  T & ArrayMap<Key, T, Allocator>::operator[](Key && key)
  {
    for (auto & p: m_data)
      if (p.first == key)
        return p.second;

    m_data.emplace_back(std::move(key), T());
    return m_data.back().second;
  }

  template <typename Key, typename T, typename Allocator>
  void ArrayMap<Key, T, Allocator>::insert(const value_type & value)
  {
    for (auto & p: m_data) {
      if (p.first == value.first) {
        p.second = value.second;
        return;
      }
    }

    m_data.push_back(value);
  }

  template <typename Key, typename T, typename Allocator>
  void ArrayMap<Key, T, Allocator>::insert(value_type && value)
  {
    for (auto & p: m_data) {
      if (p.first == value.first) {
        p.second = std::move(value.second);
        return;
      }
    }

    m_data.push_back(std::move(value));
  }

  template <typename Key, typename T, typename Allocator>
  typename ArrayMap<Key, T, Allocator>::iterator ArrayMap<Key, T, Allocator>::erase(iterator it)
  {
    auto idx = it - begin();
    *it = std::move(m_data.back());
    m_data.resize(m_data.size() - 1);
    return begin() + idx;
  }

  template <typename Key, typename T, typename Allocator>
  std::size_t ArrayMap<Key, T, Allocator>::erase(const Key & key)
  {
    for (auto & p: m_data) {
      if (p.first == key) {
        p = std::move(m_data.back());
        m_data.resize(m_data.size() - 1);
        return 1;
      }
    }
    return 0;
  }

  template <typename Key, typename T, typename Allocator>
  T ArrayMap<Key, T, Allocator>::value(const Key & key) const
  {
    for (auto & p: m_data)
      if (p.first == key)
        return p.second;
    return T();
  }

  template <typename Key, typename T, typename Allocator>
  bool ArrayMap<Key, T, Allocator>::contains(const Key & key) const
  {
    for (const auto & p: m_data)
      if (p.first == key)
        return true;
    return false;
  }

  template <typename Key, typename T, typename Allocator>
  std::size_t ArrayMap<Key, T, Allocator>::count(const Key & key) const
  {
    for (const auto & p: m_data)
      if (p.first == key)
        return 1;
    return 0;
  }

  template <typename Key, typename T, typename Allocator>
  typename ArrayMap<Key, T, Allocator>::iterator ArrayMap<Key, T, Allocator>::find(const Key & key)
  {
    for (auto it = m_data.begin(), e = m_data.end(); it != e; ++it)
      if (it->first == key)
        return it;
    return end();
  }

  template <typename Key, typename T, typename Allocator>
  typename ArrayMap<Key, T, Allocator>::const_iterator ArrayMap<Key, T, Allocator>::find(const Key & key) const
  {
    for (auto it = m_data.begin(), e = m_data.end(); it != e; ++it)
      if (it->first == key)
        return it;
    return end();
  }

  template <typename Key, typename T, typename Allocator>
  inline void swap(ArrayMap<Key, T, Allocator> & a, ArrayMap<Key, T, Allocator> & b)
  {
    a.swap(b);
  }
}

#endif // RADIANT_ARRAY_MAP_HPP
