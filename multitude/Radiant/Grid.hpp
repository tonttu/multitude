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

#ifndef RADIANT_GRID_HPP
#define RADIANT_GRID_HPP

#include <Radiant/Export.hpp>
#include <Radiant/RGBA.hpp>

#include <Nimble/Vector2.hpp>

#include <cassert>

#include <string.h>
#include <strings.h>

namespace Radiant {

  /// Grid (aka 2D array) base class with memory management
  template <class T>
  class RADIANT_API GridMemT
  {
  public:
    GridMemT(unsigned w = 0, unsigned h = 0);
    GridMemT(const GridMemT & that) : m_data(0), m_width(0), m_height(0) 
    { *this = that; }
    ~GridMemT();

    void resize(unsigned w, unsigned h);
    void resize(Nimble::Vector2i size) { resize(size.x, size.y); }
    
    /** frees up the memory, and sets the width and height of this
	object to zero. */ 
    void clear() { delete [] m_data; m_width = m_height = 0; m_data = 0; }

    void copy(T * src, unsigned w, unsigned h)
    {
      resize(w, h);
      const T * sentinel = src + w * h;
      for(T * dest = m_data; src < sentinel; ) {
        *dest = *src;
        src++;
        dest++;
      }
    }

    GridMemT & operator = (const GridMemT & that)
    {
      copy(that.m_data, that.m_width, that.m_height);
      return *this;
    }


  protected:
    T * m_data;
    unsigned m_width, m_height;
  };

  /// Grid base class without memory management
  /** This class will simply share the memory pointers with other
      objects. It is up the the user to ensure that the memory area is
      not invalidated while this object is being used. */
  template <class T>
  class RADIANT_API GridNoMemT
  {
  public:
    GridNoMemT(T * data = 0, unsigned w = 0, unsigned h = 0)
      : m_data(data), m_width(w), m_height(h)
    {}

    template <class S>
    GridNoMemT(S & that)
      : m_data(that.data()), m_width(that.width()), m_height(that.height())
    {}

    template <class S>
    GridNoMemT & operator = (S & that)
    {
      m_data = that.data();
      m_width = that.width();
      m_height = that.height();
      return * this;
    }

    void clear() { m_width = m_height = 0; m_data = 0; }
  protected:
    T * m_data;
    unsigned m_width, m_height;
  };

#if 0

#define GRID_CHECK(x,y) \
  assert((unsigned) x < this->m_width && (unsigned) y < this->m_height)
#define GRID_CHECK2(v) \
  assert((unsigned) v.x < this->m_width && (unsigned) v.y < this->m_height)

#else

#define GRID_CHECK(x,y)
#define GRID_CHECK2(v)

#endif

  /// Access to the grid elements
  template <typename T, class Base>
  class RADIANT_API GridT : public Base
  {
  public:
    typedef T * iterator;
    typedef const T * const_iterator;
    
    GridT() {}

    template <class S>
    GridT(S & that) : Base(that) {}
#ifndef WIN32
    /** Constructor that takes the elements from the data pointer,
	with given width and height. */
    GridT(T * data, unsigned w, unsigned h) : Base(data, w, h) {}
#else

    GridT(T * data, unsigned w, unsigned h)
    {
      this->m_width = w;
      this->m_height = h;
      this->m_data = data;
    }
#endif

    inline bool isInside(unsigned x, unsigned y)
    { return (x < this->m_width) && (y < this->m_height); }
    inline bool isInside(const Nimble::Vector2i & v)
    { return ((unsigned) v.x < this->m_width) &&
        ((unsigned) v.y < this->m_height); }
    inline bool isInside(const Nimble::Vector2f & v)
    { return ((unsigned) v.x < this->m_width) &&
        ((unsigned) v.y < this->m_height); }
    
    /** Gets an element from the grid. If the arguments are outside
	the grid area, then result is undefined. In certain debug
	builds, the program stops with an assertion, while on typical
	release builds the function will simply return invalid
	data. */
    inline T & get(unsigned x, unsigned y)
    { GRID_CHECK(x,y); return this->m_data[this->m_width * y + x]; }
    inline const T & get(unsigned x, unsigned y) const
    { GRID_CHECK(x,y); return this->m_data[this->m_width * y + x]; }

    inline T & get(const Nimble::Vector2i & v)
    { GRID_CHECK2(v); return this->m_data[this->m_width * v.y + v.x]; }
    inline const T & get(const Nimble::Vector2i & v) const
    { GRID_CHECK2(v); return this->m_data[this->m_width * v.y + v.x]; }

    inline T & get(const Nimble::Vector2f & v)
    { 
      GRID_CHECK2(v); 
      return this->m_data[this->m_width * (unsigned) v.y + (unsigned) v.x];
    }
    
    /** Returns an element from a grid. If the arguments are outside
	the grid area, then zero is returned. */
    inline T getSafe(const Nimble::Vector2i & v)
    { if(isInside(v)) return this->m_data[this->m_width * v.y + v.x];return 0;}
    inline T getSafe(int x, int y)
    { if(isInside(x, y)) return this->m_data[this->m_width * y + x];return 0;}

    /** Returns a reference to the grid element that is closest to the
	argument vector. */
    inline T & getNearest(const Nimble::Vector2f & v)
    { 
      unsigned x = (unsigned) (v.x+0.5f);
      unsigned y = (unsigned) (v.y+0.5f);
      GRID_CHECK(x,y);
      return this->m_data[this->m_width * y + x];
    }
    
    // Return a pointer to one line (aka row)
    inline T * line(int y)
    { return & this->m_data[this->m_width * y]; }
    // Return a clonst pointer to one line (aka row)
    inline const T * line(int y) const 
    { return & this->m_data[this->m_width * y]; }
    
    /// Writes zeroes over the memory buffer (using bzero)
    inline void zero() { bzero(this->data(), size() * sizeof(T)); }

    inline void fill(const T & val, int xlow, int ylow, int width, int height);

    /// Sets all grid elements to the given value
    inline void setAll(const T & val)
    { T * p = this->data();for(T * end = p + size(); p < end; p++) *p = val; }

    /// Returns a pointer to the data area
    inline T * data() { return this->m_data; }
    /// Returns a const pointer to the data area
    inline const T * data() const { return this->m_data; }
    /// Returns the width (number of columns) of this object
    inline unsigned width()  const { return this->m_width; }
    /// Returns the height (number of rows) of this object
    inline unsigned height() const { return this->m_height; }

    /// Number of elements
    inline unsigned size()   const { return this->m_width * this->m_height; }
    /// Returns the dimensions of the grid
    inline Nimble::Vector2i geometry() const 
    { return Nimble::Vector2i(this->m_width, this->m_height); }

    /// Checks if the width and height of this and that are identical
    template <typename S>
    bool hasIdenticalDimensions(const S & that)
    { return that.width() == width() && that.height() == height(); }

    /// Copies data from that to this using memcpy
    template <typename S>
    void copyFast(const S & that)
    { memcpy(this->m_data, that.data(), sizeof(T) * size()); }

    
  };

  
  template <typename T, class Base>
  void GridT<T, Base>::fill(const T & val, 
			    int xlow, int ylow, int width, int height)
  {
    for(int y = ylow; y <= ylow + height; y++) {
      T * dest = & get(xlow, y);
      for(T * sentinel = dest + width; dest < sentinel; dest++) {
	*dest = val;
      }
    }
  }

  typedef GridT<uint8_t, GridNoMemT<uint8_t> > PtrGrid8u;
  typedef GridT<uint8_t, GridMemT<uint8_t> >   MemGrid8u;

  typedef GridT<uint16_t, GridNoMemT<uint16_t> > PtrGrid16u;
  typedef GridT<uint16_t, GridMemT<uint16_t> >   MemGrid16u;

  typedef GridT<float, GridNoMemT<float> > PtrGrid32f;
  typedef GridT<float, GridMemT<float> >   MemGrid32f;

  typedef GridT<RGBAu8, GridNoMemT<RGBAu8> > PtrGridRGBAu8;
  typedef GridT<RGBAu8, GridMemT<RGBAu8> >   MemGridRGBAu8;

#ifdef WIN32
#ifdef RADIANT_EXPORT
  // In WIN32 template classes must be instantiated to be exported
  template class GridT<uint8_t, GridNoMemT<uint8_t>>;
  template class GridT<uint8_t, GridMemT<uint8_t>>;

  template class GridT<uint16_t, GridNoMemT<uint16_t>>;
  template class GridT<uint16_t, GridMemT<uint16_t>>;

  template class GridT<float, GridNoMemT<float>>;
  template class GridT<float, GridMemT<float>>;

  template class GridT<RGBAu8, GridNoMemT<RGBAu8>>;
  template class GridT<RGBAu8, GridMemT<RGBAu8>>;
#endif
#endif

}

#endif

