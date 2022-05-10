/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef NIMBLE_SIZE_HPP
#define NIMBLE_SIZE_HPP

#include <Nimble/Math.hpp>
#include <Nimble/Vector2.hpp>

#include <algorithm>
#include <type_traits>
#include <cassert>

#include <Qt>

namespace Nimble {

  /// This class defines the size of a two-dimensional object.
  template<typename T>
  class SizeT
  {
  public:
    /// Constructs a size with invalid width and height (i.e. isValid() returns false)
    SizeT();
    /// Construct a size defined by a 2D vector
    /// @param v vector whose components define the size
    explicit SizeT(const Nimble::Vector2T<T> & v);

    /// Constructs a size with the given width and height
    /// @param width width to initialize the size to
    /// @param height height to initialize the size to
    SizeT(T width, T height);

    /// Copy constructor
    SizeT(const SizeT &other);

    /// Returns a size holding the minimum width and height of this and the given size
    /// @param size size to compare to
    /// @return bounded size
    SizeT boundedTo(const SizeT & size) const;
    /// Returns a size holding the maximum width and height of this and the given size
    /// @param size size to compare to
    /// @return expanded size
    SizeT expandedTo(const SizeT & size) const;

    /// Check if the size is zero
    /// @return true if both the width and height is 0; otherwise false
    bool isNull() const;
    /// Check if the size is invalid
    /// @return true if both the width and height is equal to or greater than 0; otherwise false
    bool isValid() const;
    /// Check if the size is empty
    /// @return true if either of the width and height is less than or equal to 0; otherwise false
    bool isEmpty() const;

    /// Get the width
    /// @return width of the size
    T width() const { return m_width; }
    ///  Get the height
    /// @return height of the size
    T height() const { return m_height; }

    /// Fits the size to the given size
    /// @param width width to fit to
    /// @param height height to fit to
    /// @param mode either Qt::IgnoreAspectRatio or Qt::KeepAspectRatio
    void fit(T width, T height, Qt::AspectRatioMode mode);
    /// Fits the size to the given size
    /// @param size size to fit to
    /// @param mode either Qt::IgnoreAspectRatio or Qt::KeepAspectRatio
    void fit(const SizeT & size, Qt::AspectRatioMode mode);

    /// Set the width of the size
    /// @param width new width
    void setWidth(T width);
    /// Set the height of the size
    /// @param height new height
    void setHeight(T height);

    /// Set the width and height of the size
    /// @param width,height new width and height
    void make(T width, T height);

    /// @returns the smaller component
    T minimum() const;
    /// @returns the larger component
    T maximum() const;

    /// Transpose the size, i.e. swap the width and height
    void transpose();

    /// Index an element in the size object.
    /// This method does not check that the argument value is valid
    inline T operator [] (int index) const { return ((T*) this)[index]; }

    /// Returns a pointer to the first element
    inline  T * data() { return &m_width; }
    /// Returns a pointer to the first element
    inline const T * data() const { return &m_width; }

    /// Add the given size to this size
    /// @param s size to add
    /// @return reference to this size
    SizeT<T> & operator+=(const SizeT<T> & s);
    /// Subtract the given size from this size
    /// @param s size to subtract
    /// @return reference to this size
    SizeT<T> & operator-=(const SizeT<T> & s);

    /// Return the sum of two sizes. Each component is added separately.
    /// @param o size to add
    /// @return sum of sizes
    SizeT<T> operator+(const SizeT<T> & o) const;
    /// Return the subtraction of two sizes. Each component is subtracted separately.
    /// @param o size to add
    /// @return subtraction of two sizes
    SizeT<T> operator-(const SizeT<T> & o) const;

    /// Compare if two sizes are equal
    /// @param o size to compare
    /// @return true if the sizes are equal; otherwise false
    bool operator==(const SizeT<T> & o) const;
    /// Compare if two sizes are not equal
    /// @param o size to compare
    /// @return true if the sizes are not equal; otherwise false
    bool operator!=(const SizeT<T> & o) const;

    SizeT<T>& operator=(const SizeT<T> & o);

    /// Convert the size to a vector
    /// @return vector representing the size with the components set to width
    /// and height of the size
    Vector2T<T> toVector() const;

    /// Cast the vector to another type
    template<typename S>
    Nimble::SizeT<S> cast() const;

    /// Cast the vector to another type and round the values with Nimble::Math::Roundf
    template<typename S>
    Nimble::SizeT<S> round() const;

    /// Zero vector, needed to be compatible with Nimble::Vector2T
    /// @return a zero vector
    static inline SizeT<T> null() { return SizeT<T>(); }

  private:
    T m_width;
    T m_height;
  };

  /// Scale size by given scalar
  /// @param size size to scale
  /// @param scalar value to scale with
  /// @return scaled size
  template <typename T, typename U>
  inline SizeT<typename Decltype<T, U>::mul> operator*(const SizeT<T> & size, U scalar)
  {
    return SizeT<decltype(T()*U())>(size.width() * scalar, size.height() * scalar);
  }

  /// Scale size by given scalar
  /// @param scalar value to scale with
  /// @param size size to scale
  /// @return scaled size
  template <typename T, typename U>
  inline SizeT<typename Decltype<U, T>::mul> operator*(U scalar, const SizeT<T> & size)
  {
    return SizeT<decltype(U()*T())>(scalar * size.width(), scalar * size.height());
  }

  /// Divide the size component-wise by the given scalar
  /// @param size size to scale
  /// @return scaled size
  /// @param scalar value to divide with
  template <typename T, typename U>
  inline SizeT<typename Decltype<T, U>::div> operator/(const SizeT<T> & size, U scalar)
  {
    return SizeT<decltype(T()/U())>(size.width() / scalar, size.height() / scalar);
  }

  /// Divide the size component-wise by the given scalar
  /// @param size size to scale
  /// @param scalar value to divide with
  /// @return scaled size
  template <typename T, typename U>
  inline SizeT<typename Decltype<U, T>::div> operator/(U scalar, const SizeT<T> & size)
  {
    return SizeT<decltype(U()/T())>(scalar / size.width(), scalar / size.height());
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  template<typename T>
  SizeT<T>::SizeT()
    : m_width(T(-1))
    , m_height(T(-1))
  {
    // We do not support unsigned type because isValid() would not be defined
    // with them.
    static_assert(!std::is_unsigned<T>::value, "Nimble::SizeT does not support unsigned value types");
  }

  template<typename T>
  SizeT<T>::SizeT(T width, T height)
    : m_width(width)
    , m_height(height)
  {}

  template<typename T>
  SizeT<T>::SizeT(const Nimble::Vector2T<T> &v)
    : m_width(v.x)
    , m_height(v.y)
  {}

  template<typename T>
  SizeT<T>::SizeT(const SizeT &other)
    : m_width(other.m_width)
    , m_height(other.m_height)
  {
  }

  template<typename T>
  SizeT<T> SizeT<T>::boundedTo(const SizeT &size) const
  {
    return SizeT(std::min(m_width, size.m_width), std::min(m_height, size.m_height));
  }

  template<typename T>
  SizeT<T> SizeT<T>::expandedTo(const SizeT &size) const
  {
    return SizeT(std::max(m_width, size.m_width), std::max(m_height, size.m_height));
  }

  template<typename T>
  bool SizeT<T>::isNull() const
  {
    return Math::isNull(m_width) && Math::isNull(m_height);
  }

  template<typename T>
  bool SizeT<T>::isValid() const
  {
    return m_width >= T(0) && m_height >= T(0);
  }

  template<typename T>
  bool SizeT<T>::isEmpty() const
  {
    return m_width <= T(0) || m_height <= T(0);
  }

  template<typename T>
  void SizeT<T>::fit(T width, T height, Qt::AspectRatioMode mode)
  {
    fit(SizeT(width, height), mode);
  }

  template<typename T>
  void SizeT<T>::fit(const SizeT &size, Qt::AspectRatioMode mode)
  {
    assert(mode == Qt::IgnoreAspectRatio || mode == Qt::KeepAspectRatio || mode == Qt::KeepAspectRatioByExpanding);

    // Resizing an invalid size results in an invalid size
    if(!isValid())
      return;

    if(mode == Qt::IgnoreAspectRatio || Math::isNull(m_width) || Math::isNull(m_height)) {
      m_width = size.width();
      m_height = size.height();
    } else {

      bool useHeight;
      T rw = size.height() * m_width / m_height;

      if(mode == Qt::KeepAspectRatio) {
        useHeight = (rw <= size.width());
      } else {
        useHeight = (rw >= size.width());
      }

      if(useHeight) {
        m_width = rw;
        m_height = size.height();
      } else {
        m_height = size.width()  * m_height / m_width;
        m_width = size.m_width;
      }
    }
  }

  template<typename T>
  void SizeT<T>::setWidth(T width)
  {
    m_width = width;
  }

  template<typename T>
  void SizeT<T>::setHeight(T height)
  {
    m_height = height;
  }

  template<typename T>
  void SizeT<T>::make(T width, T height)
  {
    m_width = width;
    m_height = height;
  }

  template<typename T>
  T SizeT<T>::minimum() const
  {
    return std::min(m_width, m_height);
  }

  template<typename T>
  T SizeT<T>::maximum() const
  {
    return std::max(m_width, m_height);
  }

  template<typename T>
  void SizeT<T>::transpose()
  {
    std::swap(m_width, m_height);
  }

  template<typename T>
  SizeT<T> & SizeT<T>::operator+=(const SizeT<T> & s)
  {
    m_width += s.m_width;
    m_height += s.m_height;
    return *this;
  }

  template<typename T>
  SizeT<T> & SizeT<T>::operator-=(const SizeT<T> & s)
  {
    m_width -= s.m_width;
    m_height -= s.m_height;
    return *this;
  }

  /// Scales the given size
  /// @param lhs size
  /// @param s scale factor
  /// @return reference to this size
  template <typename T, typename U>
  SizeT<T> & operator*=(SizeT<T> & lhs, U s)
  {
    lhs = SizeT<T>(lhs.width() * s, lhs.height() * s);
    return lhs;
  }

  /// Divides the given size
  /// @param lhs size
  /// @param s division factor
  /// @return reference to this size
  template <typename T, typename U>
  SizeT<T> & operator/=(SizeT<T> & lhs, U s)
  {
    lhs = SizeT<T>(lhs.width() / s, lhs.height() / s);
    return lhs;
  }

  template<typename T>
  bool SizeT<T>::operator==(const SizeT<T> & o) const
  {
    return Nimble::Math::fuzzyCompare(m_width, o.m_width) &&
           Nimble::Math::fuzzyCompare(m_height, o.m_height);
  }

  template<typename T>
  bool SizeT<T>::operator!=(const SizeT<T> & o) const
  {
    return !(*this == o);
  }

  template<typename T>
  SizeT<T> & SizeT<T>::operator=(const SizeT<T> & s)
  {
    m_width = s.m_width;
    m_height = s.m_height;
    return *this;
  }

  template<typename T>
  SizeT<T> SizeT<T>::operator+(const SizeT<T> & o) const
  {
    return SizeT<T>(m_width + o.m_width, m_height + o.m_height);
  }

  template<typename T>
  SizeT<T> SizeT<T>::operator-(const SizeT<T> & o) const
  {
    return SizeT<T>(m_width - o.m_width, m_height - o.m_height);
  }

  template<typename T>
  Vector2T<T> SizeT<T>::toVector() const
  {
    return Vector2T<T>(m_width, m_height);
  }

  template<typename T>
  template<typename S>
  SizeT<S> SizeT<T>::cast() const
  {
    return SizeT<S>(S(m_width), S(m_height));
  }

  template<typename T>
  template<typename S>
  SizeT<S> SizeT<T>::round() const
  {
    return SizeT<S>(S(Nimble::Math::Roundf(m_width)), S(Nimble::Math::Roundf(m_height)));
  }

  /// Write a size into a stream
  template <class T>
  inline std::ostream &operator<<(std::ostream &os, const Nimble::SizeT<T> &t)
  {
    os << t.width() << ' ' << t.height();
    return os;
  }

  /// Read a size from a stream
  template <class T>
  inline std::istream &operator>>(std::istream &is, Nimble::SizeT<T> &t)
  {
    T x, y;
    is >> x;
    is >> y;
    t = SizeT<T>(x,y);
    return is;
  }

  /// Two-dimensional size using integer precision
  typedef SizeT<int> Size;
  /// Two-dimensional size using floating-point precision
  typedef SizeT<float> SizeF;
  /// Two-dimensional size using integer precision
  typedef SizeT<int> SizeI;

} // namespace Nimble

#endif // NIMBLE_SIZE_HPP
