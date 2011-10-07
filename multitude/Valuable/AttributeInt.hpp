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

#ifndef VALUABLE_VALUE_INT_HPP
#define VALUABLE_VALUE_INT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/AttributeNumeric.hpp>

#include <Radiant/TimeStamp.hpp>

#define VO_TYPE_INT "int"

namespace Valuable
{

  /// Template class for integer values.
  /** The actual value objects are created by using AttributeIntT<int>
      etc.

      @see AttributeInt, AttributeTimeStamp */

  template<class T>
  class VALUABLE_API AttributeIntT : public AttributeNumeric<T>
  {
    typedef AttributeNumeric<T> Base;
  public:
    using Base::value;
    using Base::m_current;
    using Base::m_values;
    using Base::m_valueSet;
    using AttributeT<T>::operator =;

    AttributeIntT() : Base() {}
    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param v The numeric value of this object
    AttributeIntT(Node * host, const QString & name, T v, bool transit = false)
        : AttributeNumeric<T>(host, name, v, transit)
    {}

    /// Assignment by subtraction
    AttributeIntT<T> & operator -= (T i) { *this = value() - i; return *this; }
    /// Assignment by addition
    AttributeIntT<T> & operator += (T i) { *this = value() + i; return *this; }
    /// Assignment by multiplication
    AttributeIntT<T> & operator *= (T i) { *this = value() * i; return *this; }
    /// Assignment by division
    AttributeIntT<T> & operator /= (T i) { *this = value() / i; return *this; }

    /// Does a logical OR for the integer
    AttributeIntT<T> & operator |= (T i) { *this = value() | i; return *this; }
    /// Does a logical AND for the integer
    AttributeIntT<T> & operator &= (T i) { *this = value() & i; return *this; }
    /// Modulo operator
    AttributeIntT<T> & operator %= (T i) { *this = value() % i; return *this; }
    /// Does a bitwise exclusive OR
    AttributeIntT<T> & operator ^= (T i) { *this = value() ^ i; return *this; }

    /// Prefix increment
    AttributeIntT<T> & operator ++ ()
    {
      if(m_current != Base::MANUAL) {
        m_values[Base::MANUAL] = m_values[m_current];
        m_valueSet[Base::MANUAL] = true;
        m_current = Base::MANUAL;
      }
      ++m_values[m_current];
      this->emitChange();
      return *this;
    }

    /// Prefix decrement
    AttributeIntT<T> & operator -- ()
    {
      if(m_current != Base::MANUAL) {
        m_values[Base::MANUAL] = m_values[m_current];
        m_valueSet[Base::MANUAL] = true;
        m_current = Base::MANUAL;
      }
      --m_values[m_current];
      this->emitChange();
      return *this;
    }

    /// Shift left
    AttributeIntT<T> & operator <<= (int i) { *this = value() << i; return *this; }
    /// Shift right
    AttributeIntT<T> & operator >>= (int i) { *this = value() >> i; return *this; }

    /// Sets the numeric value
    inline virtual bool set(int v, Attribute::Layer layer = Attribute::MANUAL,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN)
    {
      this->setValue(v, layer);
      return true;
    }
    /// @copydoc set
    inline virtual bool set(float v, Attribute::Layer layer = Attribute::MANUAL,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN)
    {
      this->setValue(v, layer);
      return true;
    }

    /// Compares less than
    bool operator < (const T & i) const { return value() < i; }
    /// Compares less or equal than
    bool operator <= (const T & i) const { return value() <= i; }
    /// Compares greater than
    bool operator > (const T & i) const { return value() > i; }
    /// Compares greater or equal than
    bool operator >= (const T & i) const { return value() >= i; }

    const char * type() const { return VO_TYPE_INT; }

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

    bool deserialize(const ArchiveElement & element);
  };

  /// 32-bit integer value object.
  typedef AttributeIntT<int32_t> AttributeInt;
  /// 32-bit integer value object.
  typedef AttributeIntT<int32_t> AttributeInt32;
  /// 32-bit unsigned integer value object.
  typedef AttributeIntT<uint32_t> AttributeUInt32;
  /// 64-bit integer value object.
  typedef AttributeIntT<int64_t> AttributeInt64;
  /// 64-bit unsigned integer value object.
  typedef AttributeIntT<uint64_t> AttributeUInt64;

  /// Time-stamp value object.
  typedef AttributeIntT<Radiant::TimeStamp> AttributeTimeStamp;

}

#ifdef __GCCXML__
/// These are exported to JS
template class Valuable::AttributeIntT<int32_t>;
#endif

#endif
