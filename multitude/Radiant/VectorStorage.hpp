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

  /// @todo Remove since std::vector actually is almost the same
  template <typename T> class VectorStorage
  {
  public:
    /// Iterator
    typedef typename std::vector<T>::iterator iterator;

    /// Creates an empty vector storage object
    VectorStorage() : m_count(0) {}

    /// Resets the internal object counter to zero.
    /** This function does not erase any objects. */
    void reset() { m_count = 0; }
    /** @copydoc reset */
    void clear() { m_count = 0; }

    /// Returns true if the vector is empty
    bool empty() const { return m_points.empty(); }

    /// Resets the internal object counter to n.
    /// @param n New value of object counter
    void truncate(size_t n) { m_count = n; }

    /// The number of objecs in the array
    size_t size() const { return m_count; }

    /// The number of allocated objects
    size_t reserved() const { return m_points.size(); }

    /// Expand the size of the storage buffer to desired size
    /** This function can be run in software initialization phase, to
    avoid the need to resize the buffer later on. */
    /// @param size new size of buffer
    void expand(size_t size)
    { if(size > m_points.size()) m_points.resize(size); }
    /// Resizes the vector
    /// @param size new size of buffer
    void resize(size_t size)
    { expand(size); m_count = size; }

    /// Gets an object, and check that the index is valid
    /** If the index is not valid, then assertion is raised, and the software stops. */
    /// @param index Index of element to retrieve
    /// @returns element from index-th position
    const T & getSafe(size_t index) const
    { assert(index < m_count); return m_points[index]; }
    /// @copydoc getSafe
    /// @param index Index of element to retrieve
    T & getSafe(size_t index)
    { assert(index < m_count); return m_points[index]; }
    /// Gets a value from the vector and expand the vector if necessary
    /// @param index Index of element to retrieve
    /// @returns A reference to the index-th element
    T & getExpand(size_t index)
    {
      if(index >= m_count) {
        expand(index + 10);
        m_count = index + 1;
      }
      return m_points[index];
    }

    /// Gets an object, without safety checks
    /// @param index Index of element to retrieve
    /// @returns Reference to the index-th element
    const T & get(size_t index) const { return m_points[index]; }
    /// @copydoc get
    /// @param index Index of element to retrieve
    /// @returns Reference to the index-th element
    T & get(size_t index) { return m_points[index]; }
    /// @returns Reference to the last element
    T & getLast() { return m_points[m_count - 1]; }
    /// @copydoc getLast
    T & last() { return m_points[m_count - 1]; }
    /// @copydoc getLast
    const T & getLast() const { return m_points[m_count - 1]; }
    /// Gets the element at size() - n - 1
    /// @param n Element-index, counted from the back
    /// @returns Reference to the n-th last element
    const T & getLast(size_t n) const { return m_points[m_count - 1 - n]; }
    /// Gets the element at size() - n - 1
    /// @param n Element-index, counted from the back
    /// @returns Reference to the n-th last element
    const T & last(size_t n) const { return m_points[m_count - 1 - n]; }

    /// Appends an object to the vector
    /** The storage area is automatically incremented if necessary. */
    /// @param x Element to append
    void append(const T & x)
    {
      if(m_count >= m_points.size())
        m_points.resize(m_count + 100);

      m_points[m_count++] = x;
    }

    /** Appends an object to the vector, equals append(x). This method
    has been implemented so that this class looks and feels more
    like a typical STL container. */
    /// @param x Element to append
    void push_back(const T & x) { append(x); }

    /// Increase the size of the storage by one, and return the last object
    /** The storage area is automatically incremented if necessary. */
    /// @returns Reference to the last object in the vector
    T & append()
    {
      if(m_count >= m_points.size())
    m_points.resize(m_count + 100);

      return m_points[m_count++];
    }

    /// Push an objec to the beginning of the array
    /** This function call takes some time on larger arrays, so it
    should be used with care. */
    /// @param x Element to prepend
    void prepend(const T & x)
    {
      if(m_count >= m_points.size())
    m_points.resize(m_count + 100);

      for(size_t i = m_count; i >= 1; i--) {
    m_points[i] = m_points[i - 1];
      }

      m_points[0] = x;
      m_count++;
    }

    /** Remove n elements from the end of the storage. */
    /// @param n Number of elements to remove
    void putBack(size_t n) { m_count -= n; }

    /** Erase an element. The size of the storage is shrunk by one. */
    /// @param index Index of element to remove
    void erase(size_t index)
    {
      for(size_t i = index + 1; i < m_count; i++)
        m_points[i - 1] = m_points[i];

      m_count--;
    }

    /** Merge elements to this from that. */
    /// @param that Vector to merge with
    void merge(VectorStorage & that)
    {
      if(!that.size()) return;

      if((size() + that.size()) > m_points.size())
        m_points.resize(size() + that.size() + 100);

      for(size_t i = 0; i < that.size(); i++)
        m_points[m_count++] = that.get(i);
    }

    /// Fills the vector with the given value
    /// @param value to fill with
    void setAll(const T & value)
    {
      for(size_t i = 0; i < size(); i++)
        m_points[i] = value;
    }

    /// @returns pointer to the first element
    T * data() { return & m_points[0]; }
    /// @copydoc data
    const T * data() const { return & m_points[0]; }

    /// Returns iterator to the beginning of the vector
    /// @returns iterator to the beginning of the vector
    iterator begin() { return m_points.begin(); }
    /// Returns iterator to the end of the vector
    /// @returns Iterator to the end of the vector
    iterator end() { return m_points.end(); }

    /// Returns the element at the given index 
    /// @returns the element at the given index 
    /// @param i Index of element to retrieve
    inline T & operator [] (size_t i) { return m_points[i]; }

    /// Returns the element at the given index 
    /// @returns the element at the given index 
    /// @param i Index of element to retrieve
    inline const T & operator [] (size_t i) const { return m_points[i]; }

    /// Swaps two VectorStorages
    /// @param that Vector to swap with
    inline void swap(VectorStorage & that)
    {
      m_points.swap(that.m_points);
      size_t tmp = m_count;
      m_count = that.m_count;
      that.m_count = tmp;
    }

    /// Copies a vector
    /// @param that Vector to copy from
    /// @returns Reference to self
    VectorStorage & operator = (const VectorStorage & that)
    {
        if(that.empty()) {
            reset();
            return * this;
        }
        else {
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
    }
    /// Returns the internal data storage area
    std::vector<T> & vector() { return m_points; }

  private:
    size_t m_count;
    std::vector<T> m_points;
  };

}

#endif

