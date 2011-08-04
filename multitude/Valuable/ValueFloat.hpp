/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_FLOAT_HPP
#define VALUABLE_VALUE_FLOAT_HPP

#include <Radiant/StringUtils.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/ValueNumeric.hpp>

#define VO_TYPE_FLOAT "float"

namespace Valuable
{
  /// Template class for floating-point values.
  /** The actual value objects are created by using ValueFloatT<float>
      etc.

      @see ValueFloat. */
  template<class T>
  class VALUABLE_API ValueFloatT : public ValueNumeric<T>
  {
    typedef ValueNumeric<T> Base;

    public:
      using Base::value;
      using ValueObjectT<T>::operator =;

      ValueFloatT() : Base() {}
      /// @copydoc ValueObject::ValueObject(HasValues *, const QString &, bool transit)
      /// @param v The numeric value of this object
      ValueFloatT(HasValues * host, const QString & name, T v = T(0), bool transit = false)
      : ValueNumeric<T>(host, name, v, transit)
      {}

      /// Assignment by subtraction
      ValueFloatT<T> & operator -= (T i) { *this = value() - i; return *this; }
      /// Assignment by addition
      ValueFloatT<T> & operator += (T i) { *this = value() + i; return *this; }
      /// Assignment by multiplication
      ValueFloatT<T> & operator *= (T i) { *this = value() * i; return *this; }
      /// Assignment by division
      ValueFloatT<T> & operator /= (T i) { *this = value() / i; return *this; }

      /// Sets the numeric value
      inline virtual bool set(int v, ValueObject::Layer layer = ValueObject::OVERRIDE)
      {
        this->setValue(v, layer);
        return true;
      }
      /// @copydoc set
      inline virtual bool set(float v, ValueObject::Layer layer = ValueObject::OVERRIDE)
      {
        this->setValue(v, layer);
        return true;
      }

      const char * type() const { return VO_TYPE_FLOAT; }

      bool deserialize(const ArchiveElement & element);

      /// @cond
      virtual void processMessage(const char * id, Radiant::BinaryData & data);
      /// @endcond

  };

  /// Float value object
  typedef ValueFloatT<float> ValueFloat;

#ifdef WIN32
#ifdef VALUABLE_EXPORT
  // In WIN32 template classes must be instantiated to be exported
  template class ValueFloatT<float>;
#endif
#endif

}

#endif
