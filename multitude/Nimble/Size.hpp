#ifndef NIMBLE_SIZE_HPP
#define NIMBLE_SIZE_HPP

#include <QSize>

#include <algorithm>
#include <type_traits>

namespace Nimble {
  
  /// This class defines the size of a two-dimensional object using integer point precision.
  template<typename T>
  class SizeT
  {
  public:
    /// Constructs a size with invalid width and height (i.e. isValid() returns false)
    SizeT();

    /// Constructs a size with the given width and height
    SizeT(int width, int height);

    /// Returns a size holding the minimum width and height of this and the given size
    SizeT boundedTo(const SizeT & size) const;
    /// Returns a size holding the maximum width and height of this and the given size
    SizeT expandedTo(const SizeT & size) const;

    /// Returns true if both the width and height is 0; otherwise false
    bool isNull() const;
    /// Returns true if both the width and height is equal to or greater than 0; otherwise false
    bool isValid() const;
    /// Returns true if either of the width and height is less than or equal to 0; otherwise false
    bool isEmpty() const;

    /// Get the width
    T width() const { return m_width; }
    ///  Get the height
    T height() const { return m_height; }

    /// Fits the size to the given size
    /// @param mode
    void fit(int width, int height, Qt::AspectRatioMode mode);
    void fit(const SizeT & size, Qt::AspectRatioMode mode);

//    SizeT scaled(int width, int height, Qt::AspectRatioMode mode) const;
//    SizeT scaled(const SizeT & size, Qt::AspectRatioMode mode) const;

    void setWidth(T width);
    void setHeight(T height);

    void transpose();
//    SizeT transposed() const;

    SizeT<T> & operator+=(const SizeT<T> & s);
    SizeT<T> & operator-=(const SizeT<T> & s);

    /// @todo fix these. for some reason forward declaration doesn't match the definition
//    template<typename U>
//    SizeT<T> & operator*=(U c);
//    template<typename U>
//    SizeT<T> & operator/=(U c);

    SizeT<T> & operator+(const SizeT<T> & o) const;
    SizeT<T> & operator-(const SizeT<T> & o) const;

    bool operator==(const SizeT<T> & o) const;
    bool operator!=(const SizeT<T> & o) const;

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
  SizeT<T>::SizeT(int width, int height)
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
  void SizeT<T>::fit(int width, int height, Qt::AspectRatioMode mode)
  {
    fit(SizeT(width, height), mode);
  }

  template<typename T>
  void SizeT<T>::fit(const SizeT &size, Qt::AspectRatioMode mode)
  {
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
    return m_width == o.m_width && m_height == o.m_height;
  }

  template<typename T>
  bool SizeT<T>::operator!=(const SizeT<T> & o) const
  {
    return !(*this == o);
  }

  typedef SizeT<int> Size;
  typedef SizeT<float> SizeF;
  
} // namespace Nimble

#endif // NIMBLE_SIZE_HPP
