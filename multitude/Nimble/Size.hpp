#ifndef NIMBLE_SIZE_HPP
#define NIMBLE_SIZE_HPP

#include <QSize>

#include <Nimble/Math.hpp>
#include <Nimble/Vector2.hpp>

#include <algorithm>
#include <type_traits>
#include <cassert>

namespace Nimble {
  
  /// This class defines the size of a two-dimensional object.
  template<typename T>
  class SizeT
  {
  public:
    /// Constructs a size with invalid width and height (i.e. isValid() returns false)
    SizeT();

    /// Constructs a size with the given width and height
    /// @param width width to initialize the size to
    /// @param height height to initialize the size to
    SizeT(T width, T height);

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

    /// Transpose the size, i.e. swap the width and height
    void transpose();

    /// Add the given size to this size
    /// @param s size to add
    /// @return reference to this size
    SizeT<T> & operator+=(const SizeT<T> & s);
    /// Subtract the given size from this size
    /// @param s size to subtract
    /// @return reference to this size
    SizeT<T> & operator-=(const SizeT<T> & s);

    /// @todo fix these. for some reason forward declaration doesn't match the definition
//    template<typename U>
//    SizeT<T> & operator*=(U c);
//    template<typename U>
//    SizeT<T> & operator/=(U c);

    /// Return the sum of two sizes. Each component is added separately.
    /// @param o size to add
    /// @return sum of sizes
    SizeT<T> & operator+(const SizeT<T> & o) const;
    /// Return the subtraction of two sizes. Each component is subtracted separately.
    /// @param o size to add
    /// @return subtraction of two sizes
    SizeT<T> & operator-(const SizeT<T> & o) const;

    /// Compare if two sizes are equal
    /// @param o size to compare
    /// @return true if the sizes are equal; otherwise false
    bool operator==(const SizeT<T> & o) const;
    /// Compare if two sizes are not equal
    /// @param o size to compare
    /// @return true if the sizes are not equal; otherwise false
    bool operator!=(const SizeT<T> & o) const;

    /// Convert the size to a vector
    /// @return vector representing the size with the components set to width
    /// and height of the size
    Vector2T<T> toVector() const;

  private:
    T m_width;
    T m_height;
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  template<typename T>
  SizeT<T>::SizeT()
    : m_width(T(-1))
    , m_height(T(-1))
  {}

  template<typename T>
  SizeT<T>::SizeT(T width, T height)
    : m_width(width)
    , m_height(height)
  {}

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
    return m_width == T(0) && m_height == T(0);
  }

  template<typename T>
  bool SizeT<T>::isValid() const
  {
    return m_width >= T(0) && m_height >= T(0);
  }

  template<typename T>
  bool SizeT<T>::isEmpty() const
  {
    return m_width < T(1) && m_height < T(1);
  }

  template<typename T>
  void SizeT<T>::fit(T width, T height, Qt::AspectRatioMode mode)
  {
    fit(SizeT(width, height), mode);
  }

  template<typename T>
  void SizeT<T>::fit(const SizeT &size, Qt::AspectRatioMode mode)
  {
    assert(mode == Qt::IgnoreAspectRatio || mode == Qt::KeepAspectRatio);

    if(mode == Qt::IgnoreAspectRatio || m_width == 0 || m_height == 0) {
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

//  template<typename T, typename U>
//  SizeT<T> & SizeT<T>::operator*=(U c)
//  {
//    static_assert(std::is_arithmetic<U>::value, "scaling Size is only defined for arithmetic types");

//    m_width *= c;
//    m_height *= c;
//    return *this;
//  }

//  template<typename T, typename U>
//  SizeT<T> & SizeT<T>::operator/=(U c)
//  {
//    static_assert(std::is_arithmetic<U>::value, "scaling Size is only defined for arithmetic types");

//    m_width /= c;
//    m_height /= c;
//    return *this;
//  }

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
  SizeT<T> & SizeT<T>::operator+(const SizeT<T> & o) const
  {
    return SizeT<T>(m_width + o.m_width, m_height + o.m_height);
  }

  template<typename T>
  SizeT<T> & SizeT<T>::operator-(const SizeT<T> & o) const
  {
    return SizeT<T>(m_width - o.m_width, m_height - o.m_height);
  }

  template<typename T>
  Vector2T<T> SizeT<T>::toVector() const
  {
    return Vector2T<T>(m_width, m_height);
  }

  /// Two-dimensional size using integer precision
  typedef SizeT<int> Size;
  /// Two-dimensional size using floating-point precision
  typedef SizeT<float> SizeF;
  
} // namespace Nimble

#endif // NIMBLE_SIZE_HPP
