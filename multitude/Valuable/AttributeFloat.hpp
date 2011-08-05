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
#include <Valuable/AttributeNumeric.hpp>

#define VO_TYPE_FLOAT "float"

namespace Valuable
{
  /// Template class for floating-point values.
  /** The actual value objects are created by using AttributeFloatT<float>
      etc.

      @see AttributeFloat. */
  template<class T>
  class VALUABLE_API AttributeFloatT : public AttributeNumeric<T>
  {
    typedef AttributeNumeric<T> Base;

    public:
      using Base::value;
      using AttributeT<T>::operator =;

      AttributeFloatT() : Base() {}
      /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
      /// @param v The numeric value of this object
      AttributeFloatT(Node * host, const QString & name, T v = T(0), bool transit = false)
      : AttributeNumeric<T>(host, name, v, transit)
      {}

      /// Assignment by subtraction
      AttributeFloatT<T> & operator -= (T i) { *this = value() - i; return *this; }
      /// Assignment by addition
      AttributeFloatT<T> & operator += (T i) { *this = value() + i; return *this; }
      /// Assignment by multiplication
      AttributeFloatT<T> & operator *= (T i) { *this = value() * i; return *this; }
      /// Assignment by division
      AttributeFloatT<T> & operator /= (T i) { *this = value() / i; return *this; }

      /// Sets the numeric value
      inline virtual bool set(int v, Attribute::Layer layer = Attribute::OVERRIDE)
      {
        this->setValue(v, layer);
        return true;
      }
      /// @copydoc set
      inline virtual bool set(float v, Attribute::Layer layer = Attribute::OVERRIDE)
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
  typedef AttributeFloatT<float> AttributeFloat;

#ifdef WIN32
#ifdef VALUABLE_EXPORT
  // In WIN32 template classes must be instantiated to be exported
  template class AttributeFloatT<float>;
#endif
#endif

}

#endif
