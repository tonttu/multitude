/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef RADIANT_VECTOR_STORAGE_HPP
#define RADIANT_VECTOR_STORAGE_HPP

#include <vector>
#include <cassert>

namespace Radiant {


  /// A container for fast array allocation/deallocation
  /** This class for handling resources that need to be frequently
      allocated/deallocated. The key point is that the memory is
      deallocated only in the destructor or when explicitly desired.

      Internally this class uses std::vector to do the real memory
      handling. It is sometimes used as a member variable of some
      class.

      Example:

      <PRE>

      VectorStorage<Item> items;

      elems.expand(5000); This is entirely optional

      Now we would use the "items" object for a longer time inside the
      while-loop.

      while(keepGoing()) {
        items.reset();

    while(fillingTheBuffer())  {
      Item item;
      items.append(item);
        }

    doSomeThingWithTheItems(items);
      }

      </PRE>

  */

  /// @todo Check if the std::vector actually is almost the same
  template <typename T> class VectorStorage
  {
  public:
    typedef typename std::vector<T>::iterator iterator;

    /// Creates an empty vector storage object
    VectorStorage() : m_count(0) {}

    /// Resets the internal object counter to zero.
    /** This function does not erase any objects. */
    void reset() { m_count = 0; }

    bool empty() const { return m_points.empty(); }

    /// Resets the internal object counter to n.
    void truncate(unsigned n) { m_count = n; }

    /// The number of objecs in the array
    unsigned size() const { return m_count; }

    /// The number of allocated objects
    unsigned reserved() const { return m_points.size(); }

    /// Expand the size of the storage buffer to desired size
    /** This function can be run in software initialization phase, to
    avoid the need to resize the buffer later on. */
    void expand(unsigned size)
    { if(size > m_points.size()) m_points.resize(size); }
    void resize(unsigned size)
    { expand(size); m_count = size; }

    /// Gets an object, and check that the index is valid
    const T & getSafe(unsigned index) const
    { assert(index < m_count); return m_points[index]; }
    T & getSafe(unsigned index)
    { assert(index < m_count); return m_points[index]; }

    T & getExpand(unsigned index)
    {
      if(index >= m_count) {
    expand(index + 10);
    m_count = index + 1;
      }
      return m_points[index];
    }

    /// Gets an object, without safety checks
    const T & get(int index) const { return m_points[index]; }
    T & get(int index) { return m_points[index]; }
    T & getLast() { return m_points[m_count - 1]; }
    T & last() { return m_points[m_count - 1]; }
    /// Ges the last object in the storage array
    const T & getLast() const { return m_points[m_count - 1]; }
    const T & getLast(int n) const { return m_points[m_count - 1 - n]; }
    const T & last(int n) const { return m_points[m_count - 1 - n]; }

    /// Appends an object to the vector
    /** The storage area is automatically incremented if necessary. */
    void append(const T & x)
    {
      if(m_count >= m_points.size())
    m_points.resize(m_count + 100);

      m_points[m_count++] = x;
    }

    /** Appends an object to the vector, equals append(x). This method
    has been implemented so that this class looks and feels more
    like a typical STL container. */
    void push_back(const T & x) { append(x); }

    /// Increase the size of the storage by one, and return the last object
    /** The storage area is automatically incremented if necessary. */
    T & append()
    {
      if(m_count >= m_points.size())
    m_points.resize(m_count + 100);

      return m_points[m_count++];
    }

    /// Push an objec to the beginning of the array
    /** This function call takes some time on larger arrays, so it
    should be used with care. */
    void prepend(const T & x)
    {
      if(m_count >= m_points.size())
    m_points.resize(m_count + 100);

      for(unsigned i = m_count; i >= 1; i--) {
    m_points[i] = m_points[i - 1];
      }

      m_points[0] = x;
      m_count++;
    }

    /** Remove n elements from the end of the storage. */
    void putBack(unsigned n) { m_count -= n; }

    /** Erase an element. The size of the storage is shrunk by one. */
    void erase(unsigned index)
    {
      for(unsigned i = index + 1; i < m_count; i++)
    m_points[i - 1] = m_points[i];

      m_count--;
    }

    /** Merge elements to this from that. */
    void merge(VectorStorage & that)
    {
      if(!that.size()) return;

      if((size() + that.size()) > m_points.size())
    m_points.resize(size() + that.size() + 100);

      for(unsigned i = 0; i < that.size(); i++)
    m_points[m_count++] = that.get(i);
    }

    void setAll(const T & value)
    {
      for(unsigned i = 0; i < size(); i++)
    m_points[i] = value;

    }

    T * data() { return & m_points[0]; }
    const T * data() const { return & m_points[0]; }

    /// Returns an iterator to the beginning of the vector
    iterator begin() { return m_points.begin(); }
    /// Returns an iterator to the end of the vector
    iterator end()
    { iterator tmp = m_points.begin(); tmp += m_count; return tmp;}

    inline T & operator [] (unsigned i) { return m_points[i]; }
    inline const T & operator [] (unsigned i) const { return m_points[i]; }

    VectorStorage & operator = (const VectorStorage & that)
    {
      m_count = that.m_count;
      expand(m_count);
      T * dest = & m_points[0];
      T * sentinel = dest + m_count;
      const T * src = & that.m_points[0];
      while(dest < sentinel) {
    *dest++ = *src++;
      }
      return * this;
    }

  private:
    unsigned m_count;
    std::vector<T> m_points;
  };

}

#endif

