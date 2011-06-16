/* COPYRIGHT
 */

#ifndef RADIANT_GRID_HPP
#define RADIANT_GRID_HPP

#include "Export.hpp"
#include "RGBA.hpp"

#include <Nimble/Vector2.hpp>

#include <cassert>

#include <string.h>
#include <strings.h>

#include <algorithm>

namespace Radiant {

  /// Grid (aka 2D array) base class with memory management
  template <class T>
  class GridMemT
  {
  public:

    /// Constructs a new grid with the given size
    GridMemT(unsigned w = 0, unsigned h = 0)
        : m_width(w), m_height(h)
    {
      unsigned s = w * h;
      if(s)
        m_data = new T[s];
      else
        m_data = 0;
    }
    /// Constructs a copy
    GridMemT(const GridMemT & that) : m_data(0), m_width(0), m_height(0)
    { *this = that; }

    ~GridMemT()
    {
        delete [] m_data;
    }

    /// Resizes this grid, by allocating new memory as necessary
    /** Any old data will be lost in this function call.

        If the number of elements in the grid stays the same, then only the
        dimensions of the grid are updated, but not the contents.

        @param w The new width of the grid
        @param h The new height of the grid
    */
    void resize(unsigned w, unsigned h)
    {
      unsigned s = w * h;
      unsigned smy = m_width * m_height;

      m_width = w;
      m_height = h;

      if(s == smy)
        return;

      delete [] m_data;

      if(s)
        m_data = new T[s];
      else
        m_data = 0;
    }

    /// Resizes the grid
    /// @see resize(unsigned w, unsigned h)
    void resize(Nimble::Vector2i size) { resize(size.x, size.y); }

    /** frees up the memory, and sets the width and height of this
    object to zero. */
    void clear() { delete [] m_data; m_width = m_height = 0; m_data = 0; }

    /// Copies data from memory
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

    /// Copies a grid
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
  };

  /// Grid base class without memory management
  /** This class will simply share the memory pointers with other
      objects. It is up the the user to ensure that the memory area is
      not invalidated while this object is being used. */
  template <class T>
  class GridNoMemT
  {
  public:
    /// Constructs a new grid with the given size
    GridNoMemT(T * data = 0, unsigned w = 0, unsigned h = 0)
      : m_data(data), m_width(w), m_height(h)
    {}

    /// Constructs a shallow copy of the grid
    template <class S>
    GridNoMemT(S & that)
      : m_data(that.data()), m_width(that.width()), m_height(that.height())
    {}

    /// Makes a shallow copy of the grid
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
  template <typename T, class Base>
  class GridT : public Base
  {
  public:
    typedef T value_type;

    /// Iterator for the grid
    typedef T * iterator;
    /// Const iterator for the grid
    typedef const T * const_iterator;

    GridT() {}

    /// Constructs a copy
    template <class S>
    GridT(S & that) : Base(that) {}

    /// @todo Is this define really needed?
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

    /// Checks if the given point is inside the grid
    inline bool isInside(unsigned x, unsigned y)
    { return (x < this->m_width) && (y < this->m_height); }
    /// Checks if the given point is inside the grid
    inline bool isInside(const Nimble::Vector2i & v)
    { return ((unsigned) v.x < this->m_width) &&
        ((unsigned) v.y < this->m_height); }
    /// Checks if the given point is inside the grid
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
    /// @copydoc get
    inline const T & get(unsigned x, unsigned y) const
    { GRID_CHECK(x,y); return this->m_data[this->m_width * y + x]; }

    /// @copydoc get
    inline T & get(const Nimble::Vector2i & v)
    { GRID_CHECK2(v); return this->m_data[this->m_width * v.y + v.x]; }
    /// @copydoc get
    inline const T & get(const Nimble::Vector2i & v) const
    { GRID_CHECK2(v); return this->m_data[this->m_width * v.y + v.x]; }
    /// @copydoc get
    inline T & get(const Nimble::Vector2f & v)
    {
      GRID_CHECK2(v);
      return this->m_data[this->m_width * (unsigned) v.y + (unsigned) v.x];
    }

    /** Gets an element from the grid. If the aruments are outside the grid area, then
        they are returned inside the image area with modulo logic. */
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



    /** Returns an element from a grid. If the arguments are outside
    the grid area, then zero is returned. */
    inline T getSafe(const Nimble::Vector2i & v)
    { if(isInside(v)) return this->m_data[this->m_width * v.y + v.x];return 0;}
    /// @copydoc getSafe
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

    /** Interpolates an element from the grid values.
        This function requires that the grid template type can be multiplied from the right
        with a floating point number. */
    inline T getInterpolated(const Nimble::Vector2f & v)
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

    /// Return a pointer to one line (aka row)
    inline T * line(int y)
    { return & this->m_data[this->m_width * y]; }
    /// Return a const pointer to one line (aka row)
    inline const T * line(int y) const
    { return & this->m_data[this->m_width * y]; }

    /// Writes zeroes over the memory buffer (using bzero)
    inline void zero() { bzero(this->data(), size() * sizeof(T)); }

    /// Fills the grid with the given value
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

    /// Swaps the contents between this grid, and the other grid
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
    for(int y = ylow; y <= ylow + height; y++) {
      T * dest = & get(xlow, y);
      for(T * sentinel = dest + width; dest < sentinel; dest++) {
    *dest = val;
      }
    }
  }

  /// A grid of bytes without memory management
  typedef GridT<uint8_t, GridNoMemT<uint8_t> > PtrGrid8u;
  /// A grid of bytes with memory management
  typedef GridT<uint8_t, GridMemT<uint8_t> >   MemGrid8u;

  /// A grid of 16-bit values without memory management
  typedef GridT<uint16_t, GridNoMemT<uint16_t> > PtrGrid16u;
  /// A grid of 16-bit values with memory management
  typedef GridT<uint16_t, GridMemT<uint16_t> >   MemGrid16u;

  /// A grid of floats without memory management
  typedef GridT<float, GridNoMemT<float> > PtrGrid32f;
  /// A grid of floats with memory management
  typedef GridT<float, GridMemT<float> >   MemGrid32f;

  /// A grid of Vector2s without memory management
  typedef GridT<Nimble::Vector2, GridNoMemT<Nimble::Vector2> > PtrGridVector2;
  /// A grid of Vector2s with memory management
  typedef GridT<Nimble::Vector2, GridMemT<Nimble::Vector2> >   MemGridVector2;

  /// A grid of color values without memory management
  typedef GridT<RGBAu8, GridNoMemT<RGBAu8> > PtrGridRGBAu8;
  /// A grid of color values with memory management
  typedef GridT<RGBAu8, GridMemT<RGBAu8> >   MemGridRGBAu8;

#ifdef WIN32
    #ifdef RADIANT_EXPORT
        template class GridT<uint8_t, GridNoMemT<uint8_t>>;
        template class GridT<uint8_t, GridMemT<uint8_t>>;

        template class GridT<uint16_t, GridNoMemT<uint16_t>>;
        template class GridT<uint16_t, GridMemT<uint16_t>>;

        template class GridT<float, GridNoMemT<float>>;
        template class GridT<float, GridMemT<float>>;

        template class GridT<Nimble::Vector2, GridNoMemT<Nimble::Vector2>>;
        template class GridT<Nimble::Vector2, GridMemT<Nimble::Vector2>>;

        template class GridT<RGBAu8, GridNoMemT<RGBAu8>>;
        template class GridT<RGBAu8, GridMemT<RGBAu8>>;
    #endif
#endif

}

#endif

