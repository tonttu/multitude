/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_GRID_HPP
#define RADIANT_GRID_HPP

#include "Radiant.hpp"
#include "Export.hpp"

#include "Memory.hpp"

#include <Nimble/Vector4.hpp>
#include <Nimble/Math.hpp>

#include <cassert>

#include <string.h>

#include <algorithm>

namespace Radiant {

  /// Grid (aka 2D array) base class with memory management
  /// @tparam T Type of objects to store
  ///
  /// Note that only simple value types are supported, meaning the following
  /// requirements must be satisfied:
  ///
  ///  * Constructors/destructors don't need to be called
  ///  * Arbitrary bits are considered as a valid, yet unspecified state
  ///  * It can be made "zero" by writing zero bytes over it
  ///
  template <class T>
  class GridMemT
  {
  public:

    /// Constructs a new grid with the given size
    /// @param w The width of the grid
    /// @param h The height of the grid
    GridMemT(unsigned w = 0, unsigned h = 0)
      : m_data(0), m_width(w), m_height(h), m_size(0)
    {
      resize(w, h);
    }
    /// Constructs a new grid with the given size
    /// @param data pointer to grid data
    /// @param w The width of the grid
    /// @param h The height of the grid
    GridMemT(void * data, unsigned w, unsigned h)
      : m_data(0), m_width(w), m_height(h), m_size(0)
    {
      resize(w, h);
      if(data != nullptr)
        memcpy(m_data, data, sizeof(T) * w * h);
    }
    /// Constructs a copy
    /// @param that Grid memory to copy
    GridMemT(const GridMemT & that) : m_data(0), m_width(0), m_height(0), m_size(0)
    { *this = that; }

    /// Destructor
    ~GridMemT()
    {
      alignedFree(m_data);
    }

    /// Resizes this grid, by allocating new memory as necessary
    /// Any old data will be lost in this function call.
    ///
    /// If the number of elements in the grid stays the same, then only the
    /// dimensions of the grid are updated, but not the contents.
    ///
    /// @param w The new width of the grid
    /// @param h The new height of the grid
    void resize(unsigned w, unsigned h)
    {
      unsigned s = w * h;

      m_width = w;
      m_height = h;

      // make the memory size dividable by 4
      while(s & 0x3) ++s;

      if(m_size >= s)
        return;

      alignedFree(m_data);
      m_size = s;

      if(s) {
        m_data = (T*)alignedMalloc(s*sizeof(T), 4096);
      } else
        m_data = 0;
    }

    /// Resizes the grid
    /// @param size New size of the grid
    /// @see resize(unsigned w, unsigned h)
    void resize(Nimble::Vector2i size) { resize(size.x, size.y); }

    /// Frees up the memory, and sets the width and height of this
    /// object to zero.
    void clear() { alignedFree(m_data); m_width = m_height = 0; m_data = 0; m_size = 0; }

    /// Copies data from memory
    /// @param src Source image data
    /// @param w Width of source image
    /// @param h Height of source image
    void copy(const T * src, unsigned w, unsigned h)
    {
      resize(w, h);
      const T * sentinel = src + w * h;
      for(T * dest = m_data; src < sentinel; ) {
        *dest = *src;
        src++;
        dest++;
      }
    }

    /// Copies a grid
    /// @param that Source grid
    /// @returns Reference to self
    GridMemT & operator = (const GridMemT & that)
    {
      copy(that.m_data, that.m_width, that.m_height);
      return *this;
    }

  protected:
    /// Pointer to the raw data
    T * m_data;
    /// Width of the grid
    unsigned m_width;
    /// Height of the grid
    unsigned m_height;
    /// Reserved data size in bytes
    unsigned m_size;
  };

  /// Grid base class without memory management
  /// This class will simply share the memory pointers with other
  /// objects. It is up the the user to ensure that the memory area is
  /// not invalidated while this object is being used.
  template <class T>
  class GridNoMemT
  {
  public:
    /// Constructs a new grid with the given size
    /// @param data Grid data
    /// @param w Width of the grid
    /// @param h Height of the grid
    GridNoMemT(T * data = 0, unsigned w = 0, unsigned h = 0)
      : m_data(data), m_width(w), m_height(h)
    {}

    /// Constructs a shallow copy of the grid
    /// @param that Object to copy
    /// @tparam S Type of target object
    template <class S>
    GridNoMemT(S & that)
      : m_data(that.data()), m_width(that.width()), m_height(that.height())
    {}

    /// Makes a shallow copy of the grid
    /// @param that Object to copy
    /// @tparam S Type of target object
    template <class S>
    GridNoMemT & operator = (S & that)
    {
      m_data = that.data();
      m_width = that.width();
      m_height = that.height();
      return * this;
    }

    /// Clears the internal variables. Does not release memory
    void clear() { m_width = m_height = 0; m_data = 0; }

  protected:
    /// Pointer to raw data
    T * m_data;
    /// Width of the grid
    unsigned m_width;
    /// Height of the grid
    unsigned m_height;
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
  /// @tparam T Type of objects to store
  /// @tparam Base Base class of this object
  template <typename T, class Base>
  class GridT : public Base
  {
  public:
    /// Type of grid data
    typedef T value_type;

    /// Iterator for the grid
    typedef T * iterator;
    /// Const iterator for the grid
    typedef const T * const_iterator;

    /// Constructor
    GridT() {}

    /// Constructs a copy
    /// @param that Object to copy
    /// @tparam S Type of object to copy
    template <class S>
    GridT(S & that) : Base(that) {}

    /// Constructor that takes the elements from the data pointer,
    /// with given width and height.
    /// @param data Grid data
    /// @param w Width of the grid
    /// @param h Height of the grid
    GridT(T * data, unsigned w, unsigned h) : Base(data, w, h) {}

    /// Checks if the given point is inside the grid
    /// @param x X-coordinate of point
    /// @param y Y-coordinate of point
    /// @returns true if the given coordinates are inside the grid
    inline bool isInside(unsigned x, unsigned y) const
    { return (x < this->m_width) && (y < this->m_height); }
    /// Checks if the given point is inside the grid
    /// @param v Point coordinate
    /// @returns true if the given coordinates are inside the grid
    template<typename U>
    bool isInside(const Nimble::Vector2T<U> & v) const
    { return isInside(v.x, v.y); }

    /// Gets an element from the grid. If the arguments are outside
    /// the grid area, then result is undefined. In certain debug
    /// builds, the program stops with an assertion, while on typical
    /// release builds the function will simply return invalid
    /// data.
    /// @param x X-coordinate of element
    /// @param y Y-coordinate of element
    /// @returns Reference to the element at (x,y)
    inline T & get(unsigned x, unsigned y)
    { GRID_CHECK(x,y); return this->m_data[this->m_width * y + x]; }

    /// @copydoc get
    inline const T & get(unsigned x, unsigned y) const
    { GRID_CHECK(x,y); return this->m_data[this->m_width * y + x]; }

    /// @copybrief get(unsigned x, unsigned y)
    /// @param v Coordinate of element
    /// @returns Reference to the element at (v.x,v.y)
    template<typename U>
    inline typename std::enable_if<std::is_integral<U>::value, T&>::type
    get(const Nimble::Vector2T<U> & v)
    { GRID_CHECK2(v); return this->m_data[this->m_width * v.y + v.x]; }

    /// @copydoc get(const Nimble::Vector2i & v)
    template<typename U>
    inline typename std::enable_if<std::is_integral<U>::value, const T&>::type
    get(const Nimble::Vector2T<U> & v) const
    { GRID_CHECK2(v); return this->m_data[this->m_width * v.y + v.x]; }

    /// @copydoc get(const Nimble::Vector2i & v)
    template<typename U>
    inline typename std::enable_if<std::is_floating_point<U>::value, T&>::type
    get(const Nimble::Vector2T<U> & v)
    { GRID_CHECK2(v); return this->m_data[this->m_width * unsigned(v.y) + unsigned(v.x)]; }

    /// @copydoc get(const Nimble::Vector2i & v)
    template<typename U>
    inline typename std::enable_if<std::is_floating_point<U>::value, const T&>::type
    get(const Nimble::Vector2T<U> & v) const
    { GRID_CHECK2(v); return this->m_data[this->m_width * unsigned(v.y) + unsigned(v.x)]; }

    /// Gets an element from the grid.
    /// @param x X-coordinate of element
    /// @param y Y-coordinate of element
    /// @returns the element at (x,y), wrapped with modulo logic.
    inline T & getCyclic(int x, int y)
    {
      x = x % (int) this->m_width;
      y = y % (int) this->m_height;
      if(x < 0)
        x += this->m_width;
      if(y < 0)
        y += this->m_height;

      return this->m_data[this->m_width * y + x];
    }

    /// Safe getter for grid data. If coordinates are invalid, this will return null element of T.
    /// @param v Coordinate of element
    /// @returns The requested element from the grid or zero if the coordinate is outside of the grid
    template<typename U>
    inline typename std::enable_if<std::is_integral<U>::value, T>::type
    getSafe(const Nimble::Vector2T<U> & v) const
    { if(isInside(v)) return this->m_data[this->m_width * v.y + v.x];return createNull<T>();}
    /// Safe getter for grid data. If coordinates are invalid, this will return null element of T.
    /// @param x X-coordinate of element
    /// @param y Y-coordinate of element
    /// @returns The requested element from the grid or zero if the coordinate is outside of the grid
    template<typename U>
    inline typename std::enable_if<std::is_integral<U>::value, T>::type
    getSafe(U x, U y) const
    { if(isInside(x, y)) return this->m_data[this->m_width * y + x];return createNull<T>();}

    /// Returns a reference to the grid element that is closest to the
    ///  argument vector.
    /// @param v Coordinate of element
    /// @return a reference to the gridpoint nearest the given coordinate
    inline T & getNearest(const Nimble::Vector2f & v)
    {
      unsigned x = (unsigned) (v.x+0.5f);
      unsigned y = (unsigned) (v.y+0.5f);
      GRID_CHECK(x,y);
      return this->m_data[this->m_width * y + x];
    }

    /// Interpolates an element from the grid values.
    /// This function requires that the grid template type can be multiplied from the right
    /// with a floating point number.
    /// @param v Coordinate of element
    /// @returns Interpolated element
    /// @tparam U Type of the result
    template<typename U>
    inline U getInterpolated(const Nimble::Vector2f & v) const
    {
      int left = v.x;
      int top = v.y;
      int right = left+1;
      int bot = top + 1;

      float wxr = v.x - left;
      float wyb = v.y - top;

      float wxl = 1.0f - wxr;
      float wyt = 1.0f - wyb;

      GRID_CHECK(left,top);
      GRID_CHECK(right,bot);

      return get(left, top) * wxl * wyt + get(right, top) * wxr * wyt +
          get(left, bot) * wxl * wyb + get(right, bot) * wxr * wyb;
    }

    /// @copybrief getInterpolated
    /// @param v Coordinate of element
    /// @returns Interpolated element
    inline T getInterpolated(const Nimble::Vector2f & v) const
    {
      return getInterpolated<T>(v);
    }

    /// Interpolates an element from the grid values.
    /// This function requires that the grid template type can be multiplied from the right
    /// with a floating point number. Ensures that the values read are inside grid.
    /// @param v Coordinate of element
    /// @returns The interpolated value from given coordinates
    /// @tparam U Type of returned objest
    template<typename U>
    inline U getInterpolatedSafe(const Nimble::Vector2f & v) const
    {
      int left = v.x;
      int top = v.y;
      int right = left+1;
      int bot = top + 1;

      float wxr = v.x - left;
      float wyb = v.y - top;

      float wxl = 1.0f - wxr;
      float wyt = 1.0f - wyb;

      int wmax = width() - 1;
      left = Nimble::Math::Clamp(left, 0, wmax);
      right = Nimble::Math::Clamp(right, 0, wmax);

      int hmax = height() - 1;
      top = Nimble::Math::Clamp(top, 0, hmax);
      bot = Nimble::Math::Clamp(bot, 0, hmax);

      return get(left, top) * wxl * wyt + get(right, top) * wxr * wyt +
          get(left, bot) * wxl * wyb + get(right, bot) * wxr * wyb;
    }

    /// @copybrief getInterpolatedSafe
    /// @param v Coordinate of element
    /// @returns The interpolated value from given coordinates
    inline T getInterpolatedSafe(const Nimble::Vector2f & v) const
    {
      return getInterpolatedSafe<T>(v);
    }

    /// Get line of the grid
    /// @returns a pointer to one line (aka row)
    /// @param y Line number
    inline T * line(int y)
    { return & this->m_data[this->m_width * y]; }
    /// Get line of the grid
    /// @returns a const pointer to one line (aka row)
    /// @param y Line number
    inline const T * line(int y) const
    { return & this->m_data[this->m_width * y]; }

    /// Writes zeroes over the memory buffer (using memset)
    inline void zero() { memset(this->data(), 0, size() * sizeof(T)); }

    /// Fills the grid with the given value
    /// @param val Value to fill with
    /// @param xlow Start x-coordinate of area to fill
    /// @param ylow Start y-coordinate of area to fill
    /// @param width Width of area to fill
    /// @param height Height of area to fill
    inline void fill(const T & val, int xlow, int ylow, int width, int height);

    /// Fills a circle in the grid with the given value
    /// @param val Value to fill with
    /// @param center The center point of the circle
    /// @param radius The radius of the circle
    inline void fillCircle(const T & val, Nimble::Vector2 center, float radius);

    /// Sets all grid elements to the given value
    /// @param val Value to fill with
    inline void setAll(const T & val)
    { T * p = this->data();for(T * end = p + size(); p < end; p++) *p = val; }

    /// Returns a pointer to the data area
    /// @return Pointer to the beginning of data
    inline T * data() { return this->m_data; }
    /// Returns a const pointer to the data area
    /// @return Pointer to the beginning of data
    inline const T * data() const { return this->m_data; }
    /// Returns the width (number of columns) of this object
    /// @return Number of columns of this grid
    inline unsigned width()  const { return this->m_width; }
    /// Returns the height (number of rows) of this object
    /// @return Number of rows of this grid
    inline unsigned height() const { return this->m_height; }

    /// Number of elements
    /// @return Number of elements inside the grid
    inline unsigned size()   const { return this->m_width * this->m_height; }
    /// Number of bytes used
    /// @return @ref size multiplied with size of stored element.
    inline unsigned sizeBytes() const { return this->size() * sizeof(T); }
    /// Returns the dimensions of the grid
    /// @return Integer vector storing width and height.
    inline Nimble::Vector2i geometry() const
    { return Nimble::Vector2i(this->m_width, this->m_height); }

    /// Checks if the width and height of this and that are identical
    /// @param that Grid to compare with
    /// @returns true if this and that have the same dimensions
    /// @tparam S Type of object to compare
    template <typename S>
    bool hasIdenticalDimensions(const S & that)
    { return that.width() == width() && that.height() == height(); }

    /// Copies data from that to this using memcpy
    /// @param that Grid to copy from
    /// @tparam S Type of object to copy
    template <typename S>
    void copyFast(const S & that)
    { memcpy(this->m_data, that.data(), sizeof(T) * size()); }

    /// Swaps the contents between this grid, and the other grid
    /// @param that Grid to swap with
    /// @tparam S Type of object to copy
    template <typename S>
    void swap(S & that)
    {
      std::swap(this->m_data, that.m_data);
      std::swap(this->m_width, that.m_width);
      std::swap(this->m_height, that.m_height);
    }

  };


  template <typename T, class Base>
  void GridT<T, Base>::fill(const T & val,
                int xlow, int ylow, int width, int height)
  {
    for(int y = ylow; y < (ylow + height); y++) {
      T * dest = & get(xlow, y);
      for(T * sentinel = dest + width; dest < sentinel; dest++) {
        *dest = val;
      }
    }
  }

  template <typename T, class Base>
  void GridT<T, Base>::fillCircle(const T & val, Nimble::Vector2 center, float radius)
  {
    int ylow = std::max((int) (center.y - radius), 0);
    int yhigh = std::min((int) (center.y + radius + 1), (int) height());

    int xlow = std::max((int) (center.x - radius), 0);
    int xhigh = std::min((int) (center.x + radius + 1), (int) width());

    for(int y = ylow; y < yhigh; y++) {
      for(int x = xlow; x < xhigh; x++) {
        float dist = (center - Nimble::Vector2(x, y)).length();
        if(dist <= radius)
          get(x, y) = val;
      }
    }
  }

  /// A read-only grid of bytes without memory management
  typedef GridT<const uint8_t, GridNoMemT<const uint8_t>> PtrGridConst8u;
  /// A grid of bytes without memory management
  typedef GridT<uint8_t, GridNoMemT<uint8_t> > PtrGrid8u;
  /// A grid of bytes with memory management
  typedef GridT<uint8_t, GridMemT<uint8_t> >   MemGrid8u;

  /// A read-only grid of 16-bit values without memory management
  typedef GridT<const uint16_t, GridNoMemT<const uint16_t>> PtrGridConst16u;
  /// A grid of 16-bit values without memory management
  typedef GridT<uint16_t, GridNoMemT<uint16_t> > PtrGrid16u;
  /// A grid of 16-bit values with memory management
  typedef GridT<uint16_t, GridMemT<uint16_t> >   MemGrid16u;

  /// A read-only grid of 32-bit values without memory management
  typedef GridT<const uint32_t, GridNoMemT<const uint32_t>> PtrGridConst32u;
  /// A grid of 32-bit values without memory management
  typedef GridT<uint32_t, GridNoMemT<uint32_t> > PtrGrid32u;
  /// A grid of 32-bit values with memory management
  typedef GridT<uint32_t, GridMemT<uint32_t> >   MemGrid32u;

  /// A read-only grid of floats without memory management
  typedef GridT<const float, GridNoMemT<const float>> PtrGridConst32f;
  /// A grid of floats without memory management
  typedef GridT<float, GridNoMemT<float> > PtrGrid32f;
  /// A grid of floats with memory management
  typedef GridT<float, GridMemT<float> >   MemGrid32f;

  /// A read-only grid of doubles without memory management
  typedef GridT<const double, GridNoMemT<const double>> PtrGridConst64f;
  /// A grid of doubles without memory management
  typedef GridT<double, GridNoMemT<double> > PtrGrid64f;
  /// A grid of doubles with memory management
  typedef GridT<double, GridMemT<double> >   MemGrid64f;

  /// A read-only grid of Vector2s without memory management
  typedef GridT<const Nimble::Vector2, GridNoMemT<const Nimble::Vector2>> PtrGridConstVector2;
  /// A grid of Vector2s without memory management
  typedef GridT<Nimble::Vector2, GridNoMemT<Nimble::Vector2> > PtrGridVector2;
  /// A grid of Vector2s with memory management
  typedef GridT<Nimble::Vector2, GridMemT<Nimble::Vector2> >   MemGridVector2;

  /// A read-only grid of Vector3s without memory management
  typedef GridT<const Nimble::Vector3, GridNoMemT<const Nimble::Vector3>> PtrGridConstVector3;
  /// A grid of Vector3s without memory management
  typedef GridT<Nimble::Vector3, GridNoMemT<Nimble::Vector3> > PtrGridVector3;
  /// A grid of Vector3s with memory management
  typedef GridT<Nimble::Vector3, GridMemT<Nimble::Vector3> >   MemGridVector3;

  /// A read-only grid of Vector4s without memory management
  typedef GridT<const Nimble::Vector4, GridNoMemT<const Nimble::Vector4>> PtrGridConstVector4;
  /// A grid of Vector4s without memory management
  typedef GridT<Nimble::Vector4, GridNoMemT<Nimble::Vector4> > PtrGridVector4;
  /// A grid of Vector4s with memory management
  typedef GridT<Nimble::Vector4, GridMemT<Nimble::Vector4> >   MemGridVector4;

#ifdef WIN32
    #ifdef RADIANT_EXPORT
        template class GridT<const uint8_t, GridNoMemT<const uint8_t>>;
        template class GridT<uint8_t, GridNoMemT<uint8_t>>;
        template class GridT<uint8_t, GridMemT<uint8_t>>;

        template class GridT<const uint16_t, GridNoMemT<const uint16_t>>;
        template class GridT<uint16_t, GridNoMemT<uint16_t>>;
        template class GridT<uint16_t, GridMemT<uint16_t>>;

        template class GridT<const uint32_t, GridNoMemT<const uint32_t>>;
        template class GridT<uint32_t, GridNoMemT<uint32_t>>;
        template class GridT<uint32_t, GridMemT<uint32_t>>;

        template class GridT<const float, GridNoMemT<const float>>;
        template class GridT<float, GridNoMemT<float>>;
        template class GridT<float, GridMemT<float>>;

        template class GridT<const double, GridNoMemT<const double>>;
        template class GridT<double, GridNoMemT<double>>;
        template class GridT<double, GridMemT<double>>;

        template class GridT<const Nimble::Vector2, GridNoMemT<const Nimble::Vector2>>;
        template class GridT<Nimble::Vector2, GridNoMemT<Nimble::Vector2>>;
        template class GridT<Nimble::Vector2, GridMemT<Nimble::Vector2>>;

        template class GridT<const Nimble::Vector3, GridNoMemT<const Nimble::Vector3>>;
        template class GridT<Nimble::Vector3, GridNoMemT<Nimble::Vector3>>;
        template class GridT<Nimble::Vector3, GridMemT<Nimble::Vector3>>;

        template class GridT<const Nimble::Vector4, GridNoMemT<const Nimble::Vector4>>;
        template class GridT<Nimble::Vector4, GridNoMemT<Nimble::Vector4>>;
        template class GridT<Nimble::Vector4, GridMemT<Nimble::Vector4>>;
    #endif
#endif

}

#endif

