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

#ifndef VALUABLE_VALUE_NUMERIC_HPP
#define VALUABLE_VALUE_NUMERIC_HPP

#include <Radiant/StringUtils.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

#define VALUEMIT_STD_OP ValueObject::emitChange(); return *this;

namespace Valuable
{
  /// A template base class for numeric values.
  /** The actual value classes are inherited from this template
      class. */
  template<class T>
  class ValueNumeric : public ValueTyped<T>
  {
    typedef ValueTyped<T> Base;

  public:
      ValueNumeric() : ValueTyped<T>() {}
      ValueNumeric(HasValues * parent, const std::string & name, T v, bool transit = false)
      : ValueTyped<T>(parent, name, v, transit)
      {}

      ValueNumeric<T> & operator = (const ValueNumeric<T> & vi) { Base::m_value = vi.m_value; VALUEMIT_STD_OP }
      ValueNumeric<T> & operator = (T i) { Base::m_value = i;  VALUEMIT_STD_OP }

      ValueNumeric<T> & operator -= (T i) { Base::m_value -= i; VALUEMIT_STD_OP }
      ValueNumeric<T> & operator += (T i) { Base::m_value += i; VALUEMIT_STD_OP }
      ValueNumeric<T> & operator *= (T i) { Base::m_value *= i; VALUEMIT_STD_OP }
      ValueNumeric<T> & operator /= (T i) { Base::m_value /= i; VALUEMIT_STD_OP }

      float asFloat(bool * const ok = 0) const { if(ok) *ok = true; return static_cast<float> (Base::m_value); }
      int asInt(bool * const ok = 0) const { if(ok) *ok = true; return static_cast<int> (Base::m_value); }
      std::string asString(bool * const ok = 0) const { if(ok) *ok = true; return Radiant::StringUtils::stringify(Base::m_value); }

      inline virtual bool set(int v) { Base::m_value = static_cast<T> (v); return true; }
      inline virtual bool set(float v) { Base::m_value = static_cast<T> (v); return true; }
  };

}

#undef VALUEMIT_STD_OP

#endif
