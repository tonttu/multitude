/* -*- C++ -*- */
/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef NIMBLE_RECT_HPP
#define NIMBLE_RECT_HPP

#include <Nimble/Export.hpp>
#include <Nimble/Matrix3.hpp>
#include <Nimble/Vector2.hpp>

namespace Nimble {

  /// An axis-aligned rectangle.
  /** The ractangle is stored as a pair of 2D vectors. The vectors
      represent the corner points of the rectangle. The "low" vector
      contains the lower X/Y values while the "high" vector contains
      the higher X/Y values.

      RectT does not really care how the coordinates are orginized
      (which way is up and so on). Some rare functions assume that one
      is using normal GUI coordinates (Y increases from top to
      bottom). */

    /// @todo rename to AARect/RectAA
  template <class T>
  class NIMBLE_API RectT
  {
  public:
    RectT()
    : m_low(0, 0),
      m_high(-1, -1)
    {}

    /// Returns true if the rectangle is empty
    inline bool isEmpty() const { return m_low.x > m_high.x || m_low.y > m_high.y; }

    /// Constructs a rectangle and initializes both corners to the given point
    RectT(const Vector2T<T> & lowHigh)
      : m_low(lowHigh), m_high(lowHigh) {}
    /// Constructs a rectangle and initializes both corners to the given point
    RectT(const Vector2T<T> * lowHigh)
      : m_low(*lowHigh), m_high(*lowHigh) {}
    /// Constructs a rectangle and initializes it to the given points
    RectT(const Vector2T<T> & low, const Vector2T<T> & high)
      : m_low(low), m_high(high) {}
    /// Constructs a rectangle and initializes it to the given points
    RectT(T xlow, T ylow, T xhigh, T yhigh)
      : m_low(xlow, ylow), m_high(xhigh, yhigh) {}
    ~RectT() {}

    /// Scales the rectangle uniformly
    void scale(T v) { m_low = m_low * v; m_high = m_high * v; }
    /// Scales the rectangle
    inline void scale(const Vector2T<T> &v);

    /// Translate the rectangle by v
    void move(const Vector2T<T> &v) { m_low += v; m_high += v; }
    /// Translate the higher corner by v but make sure it never goes below the lower corner
    void moveHighClamped(const Vector2T<T> &v)
    {
      m_high += v;
      for(int i = 0 ; i < 2; i++)
        if(m_high[i] < m_low[i])
          m_high[i] = m_low[i];
    }

    /// Resets both low and high point to origin.
    void clear() { m_low.clear(); m_high.clear(); }
    /// Resets both low and high point to the given argument point.
    void clear(const Vector2T<T> &v) { m_low = m_high = v; }

    /// Expands this rectangle to include the argument point
    inline void expand(const Vector2T<T> &v);
    /// Expands this rectangle to include the argument circle.
    inline void expand(const Vector2T<T> &v, T radius);
    /// Expands this rectangle to include the argument rectangle
    inline void expand(const RectT &b);
    /// Contracts the rectangle by v
    inline void smaller(const T & v)
    { m_low.x += v; m_low.y += v; m_high.x -= v; m_high.y -= v; }

    /// Returns the low X/Y vector
    Vector2T<T> & low() { return m_low; }
    /// Returns the low X/Y vector
    const Vector2T<T> & low() const { return m_low; }
    /// Returns the high X/Y vector
    Vector2T<T> & high() { return m_high; }
    /// Returns the high X/Y vector
    const Vector2T<T> & high() const { return m_high; }

    /** Returns the low x value combined with high y value. */
    Vector2T<T> lowHigh() const { return Vector2T<T>(m_low.x, m_high.y); }
    /** Returns the high x value combined with low y value. */
    Vector2T<T> highLow() const { return Vector2T<T>(m_high.x, m_low.y); }

    /// Sets the corner to given values
    void set(T lx, T ly, T hx, T hy)
    { m_low.make(lx, ly); m_high.make(hx, hy); }
    /// Sets the corner to given values
    void set(const Vector2T<T> &low, const Vector2T<T> &high)
    { m_low = low; m_high = high; }
    /// Sets both corners to the given value
    void set(const Vector2T<T> &point)
    { m_low = m_high = point; }

    /*void set(const Vector2T<T> &point, T radius)
    { m_low = point - radius; m_high = point + radius; }*/
    /// Sets the low corner
    void setLow(const Vector2T<T> &low) { m_low = low; }
    /// Sets the high corner
    void setHigh(const Vector2T<T> &high) { m_high = high; }
    /// Sets the x of the low corner
    void setLowX(const T lowX) { m_low.x = lowX; }
    /// Sets the y of the low corner
    void setLowY(const T lowY) { m_low.y = lowY; }
    /// Sets the x of the high corner
    void setHighX(const T highX) { m_high.x = highX; }
    /// Sets the y of the high corner
    void setHighY(const T highY) { m_high.y = highY; }

    /// Returns the center of the rectangle.
    inline Vector2T<T> center() const { return (m_low + m_high) * (T) 0.5; }

    /// Returns the vector between low and high corners
    inline Vector2T<T> span() const { return m_high - m_low; }
    /** Returns the top-center point of the rectangle. This function
    assume that we are dealing with normal GUI-coordinates where x
    increases from left to right, and y increases from top to
    bottom. */
    inline Vector2T<T> topCenter() const;

    /// Returns the width of the rectangle
    inline T width()  const { return m_high.x - m_low.x; }
    /// Returns the height of the rectangle
    inline T height() const { return m_high.y - m_low.y; }
    /// Returns the size of the rectangle (= high - low)
    inline Vector2T<T> size() const { return m_high - m_low; }
    /// Returns the surface area of the rectangle
    inline T area() const { Vector2T<T> s(size()); return s.x * s.y; }

    inline RectT intersection(const RectT &) const;

    /// Check if two rectangles intersect
    inline bool intersects(const RectT &) const;
    /// Check if the rectangle contains the given point
    inline bool contains(T x, T y) const;
    /// Check if the rectangle contains the given point
    inline bool contains(Vector2T<T> v) const;
    /// Check if the rectangle contains the given rectangle
    inline bool contains(const RectT &b) const;
    /// Compute the distance to the other rectangle
    inline T    distance(const RectT &b) const;

    /// Clamps the argument vector to be inside this rectangle
    inline Vector2T<T> clamp(const Vector2T<T> &) const;

    /// Transforms the rectangle with the given matrix
    inline void transform(const Matrix3T<T>& m);
    /// Scales the rectangle
    inline void shrinkRelative(float xs, float ys);
    /// Increases the size of the rectangle uniformly
    /// @todo duplicate with smaller() mostly (make a single function that works with negative values)
    inline void increaseSize(T add)
    { m_low.x -= add; m_low.y -= add; m_high.x += add; m_high.y += add; }

    /** Returns one quarter of the rectangle.

    @arg row The row of the quarter (0-1)
    @arg col The column of the quarter (0-1)
    */
    inline RectT quarter(int row, int col) const;

    inline bool operator == (const RectT<T> & o) const {
      return m_low == o.m_low && m_high == o.m_high;
    }

    /// Returns a const pointer to the rectangle corner data
    const T * data() const { return m_low.data(); }
    /// Returns a pointer to the rectangle corner data
    T * data() { return m_low.data(); }

  private:
    Vector2T<T> m_low, m_high;
  };

  template <class T>
  inline void RectT<T>::expand(const Vector2T<T> &v)
  {
      if(isEmpty()) {
          *this = RectT<T>(v);
      } else {

          if(v[0] < m_low[0]) m_low[0] = v[0];
          if(v[1] < m_low[1]) m_low[1] = v[1];

          if(v[0] > m_high[0]) m_high[0] = v[0];
          if(v[1] > m_high[1]) m_high[1] = v[1];
      }
  }

  template <class T>
  inline void RectT<T>::expand(const Vector2T<T> &v, T radius)
  {
    expand(v - Vector2T<T>(radius, radius));
    expand(v + Vector2T<T>(radius, radius));
  }

  template <class T>
  inline void RectT<T>::expand(const RectT &b)
  {
    if(isEmpty()) {
      *this = b;
      return;
    }

    if(b.isEmpty())
      return;

    if(b.m_low[0] < m_low[0]) m_low[0] = b.m_low[0];
    if(b.m_low[1] < m_low[1]) m_low[1] = b.m_low[1];

    if(b.m_high[0] > m_high[0]) m_high[0] = b.m_high[0];
    if(b.m_high[1] > m_high[1]) m_high[1] = b.m_high[1];
  }

  template <class T>
  void RectT<T>::scale(const Vector2T<T> &v)
  {
    m_low[0] *= v[0];
    m_low[1] *= v[1];

    m_high[0] *= v[0];
    m_high[1] *= v[1];
  }

  template <class T>
  Vector2T<T> RectT<T>::topCenter() const
  {
    return Vector2T<T>((m_low.x + m_high.x) * T(0.5), m_high.y);
  }

  template <class T>
  RectT<T> RectT<T>::intersection(const RectT & b) const
  {
    RectT<T> ret;
    for(int i = 0; i < 2; i++) {
      ret.m_low[i] = Math::Max(m_low[i], b.m_low[i]);
      ret.m_high[i] = Math::Min(m_high[i], b.m_high[i]);
    }
    return ret;
  }

  template <class T>
  bool RectT<T>::intersects(const RectT &b) const
  {
    for(int i = 0; i < 2; i++) {
      if(b.m_high[i] < m_low[i] || b.m_low[i] > m_high[i])
    return false;
    }

    return true;
  }

  template <class T>
  inline bool RectT<T>::contains(T x, T y) const
  {
    return ((x >= m_low[0]) && (x <= m_high[0]) &&
        (y >= m_low[1]) && (y <= m_high[1]));
  }

  template <class T>
  inline bool RectT<T>::contains(Vector2T<T> v) const
  {
    return ((v[0] >= m_low[0]) && (v[0] <= m_high[0])
      && (v[1] >= m_low[1]) && (v[1] <= m_high[1]));
  }

  template <class T>
  inline bool RectT<T>::contains(const RectT &b) const
  {
    return ((b.m_low[0] >= m_low[0]) && (b.m_high[0] <= m_high[0]) &&
        (b.m_low[1] >= m_low[1]) && (b.m_high[1] <= m_high[1]));
  }

  template <class T>
  inline T RectT<T>::distance(const RectT &b) const
  {
    Vector2T<T> mind;

    for(int i = 0; i < 2; i++) {

      if(b.m_high[i] < m_low[i])
        mind[i] = m_low[i] - b.m_high[i];
      else if(b.m_low[i] > m_high[i])
        mind[i] = b.m_low[i] - m_high[i];
      else
        mind[i] = 0;

    }

    return mind.maximum();
  }

  template <class T>
  inline Vector2T<T> RectT<T>::clamp(const Vector2T<T> &v) const
  {
    int i;
    Vector2T<T> r(v);

    for(i=0; i < 2; i++)
      if(r[i] < m_low[i]) r[i] = m_low[i];

    for(i=0; i < 2; i++)
      if(r[i] > m_high[i]) r[i] = m_high[i];

    return r;
  }

  /// @cond
  template <>
  inline Vector2T<int> RectT<int>::center() const
  {
    return (m_low + m_high) / 2;
  }
  /// @endcond

  template<class T>
  void RectT<T>::transform(const Matrix3T<T>& m)
  {
    using Math::Min;
    using Math::Max;
    Vector2T<T> v0(m_low.x, m_low.y);
    Vector2T<T> v1(m_high.x, m_low.y);
    Vector2T<T> v2(m_high.x, m_high.y);
    Vector2T<T> v3(m_low.x, m_high.y);

    Vector2T<T> t0 = m.project(v0);
    Vector2T<T> t1 = m.project(v1);
    Vector2T<T> t2 = m.project(v2);
    Vector2T<T> t3 = m.project(v3);

    m_low.x = Min(Min(Min(t0.x, t1.x), t2.x), t3.x);
    m_low.y = Min(Min(Min(t0.y, t1.y), t2.y), t3.y);
    m_high.x = Max(Max(Max(t0.x, t1.x), t2.x), t3.x);
    m_high.y = Max(Max(Max(t0.y, t1.y), t2.y), t3.y);
  }

  template<class T>
  inline void RectT<T>::shrinkRelative(float xs, float ys)
  {
    float w = m_high.x - m_low.x;
    float h = m_high.y - m_low.y;

    float wloss = 0.5f * w * xs;
    float hloss = 0.5f * h * ys;

    m_low.x += wloss;
    m_high.x -= wloss;
    m_low.y += hloss;
    m_high.y -= hloss;
  }

  template<class T>
  inline RectT<T> RectT<T>::quarter(int row, int col) const
  {
    RectT<T> res;

    const Vector2T<T> sp(span());
    Vector2T<T> size(sp.x / T(2), sp.y / T(2));
    if(row)
      res.m_low.y = m_low.y + size.y;
    if(col)
      res.m_low.x = m_low.x + size.x;
    res.m_high = res.m_low + size;
    return res;
  }

  /// Rectangle of floats
  typedef RectT<float> Rect;
  /// Rectangle of floats
  typedef RectT<float> Rectf;
  /// Rectangle of ints
  typedef RectT<int> Recti;
  /// Rectangle of doubles
  typedef RectT<double> Rectd;

  // These are needed under Windows
  #ifdef WIN32
  #   ifdef NIMBLE_EXPORT
          template Nimble::RectT<float>;
          template Nimble::RectT<int>;
          template Nimble::RectT<double>;
  #   endif
  #endif

}

#endif
